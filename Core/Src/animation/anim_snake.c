#include "anim_snake.h"
#include <stdint.h>
#include <string.h>

/* extern v projektu (pokud nejsou v hlavičce) */
// extern Voxel_t scrBuf[8][8][8];
// uint32_t HAL_GetTick(void);

/* ---------------------------- Parametry ---------------------------- */
#define GRID_N            8
#define SNAKE_START_LEN   4
#define SNAKE_WIN_LEN     40     /* výhra při délce 40 */
#define TICK_MS           134    /* ~+20% zpomalení */

#define WIN_TICKS         18
#define FAIL_TICKS        10

/* -------------------------- Stav & RNG ----------------------------- */
typedef struct { int8_t x,y,z; } vec3b;

typedef enum { MODE_PLAY=0, MODE_WIN, MODE_FAIL } mode_t;

static struct {
    mode_t  mode;
    uint8_t anim_ticks;

    vec3b   body[64];
    int     len;

    vec3b   food;
    Voxel_t food_col;      /* barva jídla */
    Voxel_t snake_col;     /* aktuální barva hada (tělo) */

    vec3b   last_dir;      /* poslední směr (pro vykreslování/feel; BFS ho nepotřebuje) */

    uint32_t rng;          /* LCG seed */
} S;

static inline uint8_t rr8(void){
    S.rng = S.rng*1664525u + 1013904223u;
    return (uint8_t)(S.rng >> 24);
}

/* -------------------------- Utility -------------------------------- */
static inline int in_bounds(int x,int y,int z){
    return (unsigned)x < GRID_N && (unsigned)y < GRID_N && (unsigned)z < GRID_N;
}
static inline int eq_pos(vec3b a, vec3b b){
    return a.x==b.x && a.y==b.y && a.z==b.z;
}

/* náhodná živá barva (vyhne se šedé/bílé) */
static Voxel_t rand_color(void){
    Voxel_t c;
    uint8_t hi  = (uint8_t)(200 + (rr8() % 56));
    uint8_t mid = (uint8_t)( 80 + (rr8() % 121));
    uint8_t lo  = (uint8_t)( rr8() % 41);
    switch (rr8() % 6) {
        case 0: c.r=hi;  c.g=mid; c.b=lo;  break;
        case 1: c.r=hi;  c.g=lo;  c.b=mid; break;
        case 2: c.r=mid; c.g=hi;  c.b=lo;  break;
        case 3: c.r=lo;  c.g=hi;  c.b=mid; break;
        case 4: c.r=mid; c.g=lo;  c.b=hi;  break;
        default:c.r=lo;  c.g=mid; c.b=hi;  break;
    }
    return c;
}

/* lehce zesvětli barvu (pro hlavu) */
static inline Voxel_t brighten(Voxel_t c, uint8_t add){
    int r=c.r+add, g=c.g+add, b=c.b+add;
    if (r>255) r=255;
    if (g>255) g=255;
    if (b>255) b=255;
    return (Voxel_t){ (uint8_t)r,(uint8_t)g,(uint8_t)b };
}

static int occupied(int x,int y,int z, int tail_exempt){
    int upto = S.len - (tail_exempt ? 1 : 0);
    for (int i=0;i<upto;i++) if (S.body[i].x==x && S.body[i].y==y && S.body[i].z==z) return 1;
    return 0;
}

/* spawn jídla + barvy */
static void spawn_food(void){
    int tries;
    for (tries=0; tries<64; ++tries){
        int x = rr8() & 7, y = rr8() & 7, z = rr8() & 7;
        if (!occupied(x,y,z,0)){ S.food=(vec3b){x,y,z}; S.food_col = rand_color(); return; }
    }
    for (int z=0; z<GRID_N; ++z){
        for (int y=0; y<GRID_N; ++y){
            for (int x=0; x<GRID_N; ++x){
                if (!occupied(x,y,z,0)){ S.food=(vec3b){x,y,z}; S.food_col = rand_color(); return; }
            }
        }
    }
}

/* inicializace nové hry */
static void game_reset(void){
    memset(&S, 0, sizeof(S));
    S.mode = MODE_PLAY;
    S.anim_ticks = 0;
    S.rng = HAL_GetTick();

    /* start: (2,2,2) → +X, délka 4 */
    {
        int sx=2, sy=2, sz=2, i;
        S.len = SNAKE_START_LEN;
        for (i=0;i<S.len;i++) S.body[i]=(vec3b){ (int8_t)(sx+i), (int8_t)sy, (int8_t)sz };
        S.last_dir = (vec3b){1,0,0};
    }

    S.snake_col = (Voxel_t){0,180,40}; /* výchozí zelená */
    spawn_food();
}

/* Manhattan vzdálenost */
static inline int manhattan(vec3b a, vec3b b){
    int dx = a.x - b.x; if (dx<0) dx=-dx;
    int dy = a.y - b.y; if (dy<0) dy=-dy;
    int dz = a.z - b.z; if (dz<0) dz=-dz;
    return dx+dy+dz;
}

/* ----- BFS pathfinding na 8×8×8 (s výjimkou ocasu) ----- */
static inline int idx_of(int x,int y,int z){ return x + (y<<3) + (z<<6); }
static inline void coords_of(int idx, int *x,int *y,int *z){
    *x = idx & 7; *y = (idx >> 3) & 7; *z = (idx >> 6) & 7;
}

/* Vrátí doporučený další krok (dx,dy,dz) podle BFS. Pokud cesta neexistuje,
   vrátí (0,0,0) a vyšší logika zkusí fallback. */
static vec3b bfs_next_step(vec3b head, vec3b goal){
    /* statické pracovní buffery (šetří stack) */
    static int16_t parent[512];
    static uint16_t queue[512];
    static uint8_t blocked[512];

    int i, head_idx, goal_idx, qh, qt;

    /* naplň obsazenost – všechny články hada jako překážky,
       ale OCAS povolíme (tail_exempt), protože se hned posune */
    for (i=0;i<512;i++) blocked[i]=0;
    for (i=0;i<S.len;i++){
        int bi = idx_of(S.body[i].x, S.body[i].y, S.body[i].z);
        blocked[bi] = 1;
    }
    /* povol tail buňku jako volnou (pokud had nemá délku 0) */
    if (S.len > 0){
        int tail_idx = idx_of(S.body[0].x, S.body[0].y, S.body[0].z);
        blocked[tail_idx] = 0;
    }

    head_idx = idx_of(head.x, head.y, head.z);
    goal_idx = idx_of(goal.x, goal.y, goal.z);

    /* goal může být v těle – jídlo nikdy nespawnujeme do těla, ale pro jistotu: */
    blocked[head_idx] = 0;
    blocked[goal_idx] = 0;

    for (i=0;i<512;i++) parent[i] = -1;

    /* BFS queue init */
    qh = 0; qt = 0;
    queue[qt++] = (uint16_t)head_idx;
    parent[head_idx] = head_idx;

    /* Pořadí sousedů: preferuj poslední směr (méně zatáček), pak ostatní osy */
    vec3b pref[6];
    {
        /* vytvoř stabilní pořadí sousedů */
        int n = 0;
        vec3b dirs[6] = { {+1,0,0},{-1,0,0},{0,+1,0},{0,-1,0},{0,0,+1},{0,0,-1} };
        /* nejdřív last_dir, pokud je nenulový */
        if (!(S.last_dir.x==0 && S.last_dir.y==0 && S.last_dir.z==0)){
            pref[n++] = S.last_dir;
        }
        /* pak směry k cíli (správná znaménka os) – přidáme je, pokud tam ještě nejsou */
        int sx = (goal.x > head.x) ? +1 : (goal.x < head.x) ? -1 : 0;
        int sy = (goal.y > head.y) ? +1 : (goal.y < head.y) ? -1 : 0;
        int sz = (goal.z > head.z) ? +1 : (goal.z < head.z) ? -1 : 0;
        vec3b targ[3] = { {sx,0,0}, {0,sy,0}, {0,0,sz} };
        for (i=0;i<3;i++){
            if (targ[i].x==0 && targ[i].y==0 && targ[i].z==0) continue;
            int dup=0, k;
            for (k=0;k<n;k++) if (pref[k].x==targ[i].x && pref[k].y==targ[i].y && pref[k].z==targ[i].z){ dup=1; break; }
            if (!dup) pref[n++] = targ[i];
        }
        /* doplň zbytek směrů */
        for (i=0;i<6;i++){
            int dup=0, k;
            for (k=0;k<n;k++) if (pref[k].x==dirs[i].x && pref[k].y==dirs[i].y && pref[k].z==dirs[i].z){ dup=1; break; }
            if (!dup) pref[n++] = dirs[i];
        }
        /* pokud by n<6, doplň, ale nemělo by nastat */
        // while (n<6){ pref[n++] = dirs[n-1]; }
    }

    /* BFS smyčka */
    while (qh < qt){
        int cur = queue[qh++];
        if (cur == goal_idx) break;

        int cx, cy, cz;
        coords_of(cur, &cx,&cy,&cz);

        /* projdi 6 sousedů v preferovaném pořadí */
        for (i=0;i<6;i++){
            int nx = cx + pref[i].x;
            int ny = cy + pref[i].y;
            int nz = cz + pref[i].z;
            if (!in_bounds(nx,ny,nz)) continue;
            {
                int ni = idx_of(nx,ny,nz);
                if (parent[ni] != -1) continue;   /* už navštíveno */
                if (blocked[ni]) continue;        /* překážka */
                parent[ni] = (int16_t)cur;
                queue[qt++] = (uint16_t)ni;
                if (qt >= 512) break;             /* bezpečnost */
            }
        }
        if (qt >= 512) break;
    }

    if (parent[goal_idx] == -1){
        /* žádná cesta */
        vec3b zero = {0,0,0};
        return zero;
    }

    /* zrekonstruuj první krok na cestě head -> goal */
    {
        int step = goal_idx;
        int prev = parent[step];
        while (prev != head_idx){
            step = prev;
            prev = parent[step];
        }
        /* rozdíl coords(step) - head -> směr */
        {
            int sx, sy, sz;
            coords_of(step, &sx,&sy,&sz);
            vec3b dir = { (int8_t)(sx - head.x), (int8_t)(sy - head.y), (int8_t)(sz - head.z) };
            /* jistota – zaručeně jednotkový v jedné ose */
            return dir;
        }
    }
}

/* fallback: když BFS selže – vyber „nejméně špatný“ bezpečný krok */
static vec3b fallback_step(vec3b head, vec3b goal){
    vec3b best = {0,0,0};
    int best_md = 1000;

    /* preferuj pokračování stejným směrem, pokud je bezpečné */
    if (!(S.last_dir.x==0 && S.last_dir.y==0 && S.last_dir.z==0)){
        vec3b nh = { (int8_t)(head.x + S.last_dir.x),
                     (int8_t)(head.y + S.last_dir.y),
                     (int8_t)(head.z + S.last_dir.z) };
        if (in_bounds(nh.x,nh.y,nh.z) && !occupied(nh.x,nh.y,nh.z, 1)){
            best = S.last_dir;
            best_md = manhattan(nh, goal);
        }
    }

    /* projdi všechny směry a vezmi ten s nejmenší md (bez kolize) */
    {
        vec3b dirs[6] = { {+1,0,0},{-1,0,0},{0,+1,0},{0,-1,0},{0,0,+1},{0,0,-1} };
        int i;
        for (i=0;i<6;i++){
            vec3b d = dirs[i];
            vec3b nh = { (int8_t)(head.x + d.x),
                         (int8_t)(head.y + d.y),
                         (int8_t)(head.z + d.z) };
            if (!in_bounds(nh.x,nh.y,nh.z)) continue;
            if (occupied(nh.x,nh.y,nh.z, 1)) continue;
            {
                int md = manhattan(nh, goal);
                if (md < best_md){
                    best = d; best_md = md;
                }
            }
        }
    }
    return best;
}

/* posuň hada; 1=ok, 2=eat, 0=fail */
static int snake_step(void){
    vec3b head = S.body[S.len-1];

    /* nejdřív BFS; pokud selže, fallback */
    vec3b d = bfs_next_step(head, S.food);
    if (d.x==0 && d.y==0 && d.z==0){
        d = fallback_step(head, S.food);
        if (d.x==0 && d.y==0 && d.z==0) return 0; /* žádný bezpečný krok */
    }

    /* zapamatuj pro „plynulost“ (jen kosmetika) */
    S.last_dir = d;

    /* vypočti novou hlavu */
    {
        vec3b nh = { (int8_t)(head.x + d.x),
                     (int8_t)(head.y + d.y),
                     (int8_t)(head.z + d.z) };
        if (!in_bounds(nh.x,nh.y,nh.z)) return 0;

        /* sním jídlo? */
        {
            int eat = eq_pos(nh, S.food);

            /* kolize do těla (když nejím, ocas se posune; BFS už s tím počítal, ale zkontrolujme) */
            if (!eat && occupied(nh.x,nh.y,nh.z, 0)) return 0;

            if (eat){
                if (S.len < (int)(sizeof(S.body)/sizeof(S.body[0]))){
                    S.body[S.len] = nh; S.len++;
                } else {
                    memmove(&S.body[0], &S.body[1], (S.len-1)*sizeof(vec3b));
                    S.body[S.len-1] = nh;
                }
                /* přebarvi hada na barvu jídla a spawnni nové jídlo */
                S.snake_col = S.food_col;
                spawn_food();
                return 2;
            } else {
                memmove(&S.body[0], &S.body[1], (S.len-1)*sizeof(vec3b));
                S.body[S.len-1] = nh;
                return 1;
            }
        }
    }
}

/* vykreslení */
static inline void put(int x,int y,int z, Voxel_t c){
    if ((unsigned)x>7||(unsigned)y>7||(unsigned)z>7) return;
    scrBuf[x][y][z] = c;
}

static void render_win_frame(uint8_t t){
    uint8_t amp = (t & 1) ? 255 : 180;
    int x,y,z;
    for (x=0;x<8;x++)
        for (y=0;y<8;y++)
            for (z=0;z<8;z++){
                uint8_t g = (uint8_t)((amp * (z+1)) / 8);
                scrBuf[x][y][z] = (Voxel_t){0,g,0};
            }
}
static void render_fail_frame(uint8_t t){
    uint8_t r = (t & 1) ? 255 : 120;
    int x,y,z;
    for (x=0;x<8;x++)
        for (y=0;y<8;y++)
            for (z=0;z<8;z++){
                scrBuf[x][y][z] = (Voxel_t){r,0,0};
            }
}

/* --------------------------- Hlavní animace ------------------------ */
void anim_snake(graph_animation_t *a)
{
    if (a->reset){
      game_reset();
      a->reset = 0;
    }

    a->timer = HAL_GetTick() + TICK_MS;

    if (S.mode == MODE_WIN){
        render_win_frame(S.anim_ticks++);
        if (S.anim_ticks >= WIN_TICKS) game_reset();
        return;
    } else if (S.mode == MODE_FAIL){
        render_fail_frame(S.anim_ticks++);
        if (S.anim_ticks >= FAIL_TICKS) game_reset();
        return;
    }

    /* logika hry */
    {
        int step_result = snake_step();
        if (step_result == 0){ S.mode = MODE_FAIL; S.anim_ticks = 0; return; }
        if (S.len >= SNAKE_WIN_LEN){ S.mode = MODE_WIN; S.anim_ticks = 0; return; }
    }

    /* vykreslení scény */
    memset(scrBuf, 0, sizeof(scrBuf));

    /* jídlo (náhodná barva) */
    put(S.food.x, S.food.y, S.food.z, S.food_col);

    /* tělo = barva hada, hlava = jasnější */
    {
        Voxel_t bodyC = S.snake_col;
        Voxel_t headC = brighten(S.snake_col, 40);
        int i;
        for (i=0;i<S.len-1;i++) put(S.body[i].x, S.body[i].y, S.body[i].z, bodyC);
        {
            vec3b h = S.body[S.len-1];
            put(h.x, h.y, h.z, headC);
        }
    }
}

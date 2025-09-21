#include "anim_shrink_cube.h"
#include <stdint.h>
#include <string.h>

/* extern v projektu (pokud nejsou v hlavičce) */
// extern Voxel_t scrBuf[8][8][8];
// uint32_t HAL_GetTick(void);

typedef struct {
    uint8_t inited;
    uint8_t corner_idx;   // 0..7
    int8_t  size;         // 7..1 (1 => 2x2x2 box), pak zpět 1..7
    int8_t  dir;          // -1 = shrink, +1 = expand
    uint32_t rng;         // LCG seed
    uint8_t  cr, cg, cb;  // aktuální barva hran
    uint8_t  prev_r, prev_g, prev_b; // barva z minulého cyklu
} shrink_state_t;

static shrink_state_t st;

/* 8 rohů v pořadí „dokola“ */
static const uint8_t corners[8][3] = {
    {0,0,0},{7,0,0},{7,7,0},{0,7,0},
    {0,0,7},{7,0,7},{7,7,7},{0,7,7}
};

/* --- LCG RNG --- */
static inline uint8_t rr8(void){
    st.rng = st.rng * 1664525u + 1013904223u;
    return (uint8_t)(st.rng >> 24);
}

/* Slušná „náhodná sytá“ barva (vyhne se bílé/šedé) */
static void pick_random_color(uint8_t *r, uint8_t *g, uint8_t *b){
    uint8_t hi = (uint8_t)(200 + (rr8() % 56));
    uint8_t mid= (uint8_t)(80  + (rr8() % 121));
    uint8_t lo = (uint8_t)(rr8() % 41);
    switch (rr8() % 6) {
        case 0: *r=hi;  *g=mid; *b=lo;  break;
        case 1: *r=hi;  *g=lo;  *b=mid; break;
        case 2: *r=mid; *g=hi;  *b=lo;  break;
        case 3: *r=lo;  *g=hi;  *b=mid; break;
        case 4: *r=mid; *g=lo;  *b=hi;  break;
        default:*r=lo;  *g=mid; *b=hi;  break;
    }
}

/* Nově: vyber barvu odlišnou od minulé (prev_*) */
static void pick_random_color_distinct(uint8_t *r, uint8_t *g, uint8_t *b){
    for (int tries=0; tries<10; ++tries){
        uint8_t rr,gg,bb;
        pick_random_color(&rr,&gg,&bb);
        if (rr!=st.prev_r || gg!=st.prev_g || bb!=st.prev_b) {
            *r=rr; *g=gg; *b=bb;
            return;
        }
    }
    /* kdyby to „náhodou“ 10× trefilo stejnou, vezmeme poslední */
    pick_random_color(r,g,b);
}

static inline void set_vox(int x,int y,int z, uint8_t r,uint8_t g,uint8_t b){
    if ((unsigned)x>7||(unsigned)y>7||(unsigned)z>7) return;
    scrBuf[x][y][z].r = r;
    scrBuf[x][y][z].g = g;
    scrBuf[x][y][z].b = b;
}

static inline void draw_edge_X(int x0,int x1,int y,int z, uint8_t r,uint8_t g,uint8_t b){
    if ((unsigned)y>7||(unsigned)z>7) return;
    if (x0>x1){ int t=x0; x0=x1; x1=t; }
    if (x0<0) x0=0; if (x1>7) x1=7;
    for (int x=x0; x<=x1; ++x) set_vox(x,y,z,r,g,b);
}
static inline void draw_edge_Y(int y0,int y1,int x,int z, uint8_t r,uint8_t g,uint8_t b){
    if ((unsigned)x>7||(unsigned)z>7) return;
    if (y0>y1){ int t=y0; y0=y1; y1=t; }
    if (y0<0) y0=0; if (y1>7) y1=7;
    for (int y=y0; y<=y1; ++y) set_vox(x,y,z,r,g,b);
}
static inline void draw_edge_Z(int z0,int z1,int x,int y, uint8_t r,uint8_t g,uint8_t b){
    if ((unsigned)x>7||(unsigned)y>7) return;
    if (z0>z1){ int t=z0; z0=z1; z1=t; }
    if (z0<0) z0=0; if (z1>7) z1=7;
    for (int z=z0; z<=z1; ++z) set_vox(x,y,z,r,g,b);
}
static inline void draw_box_edges(int minx,int miny,int minz, int maxx,int maxy,int maxz,
                                  uint8_t r,uint8_t g,uint8_t b)
{
    draw_edge_X(minx,maxx, miny,minz, r,g,b);
    draw_edge_X(minx,maxx, miny,maxz, r,g,b);
    draw_edge_X(minx,maxx, maxy,minz, r,g,b);
    draw_edge_X(minx,maxx, maxy,maxz, r,g,b);

    draw_edge_Y(miny,maxy, minx,minz, r,g,b);
    draw_edge_Y(miny,maxy, minx,maxz, r,g,b);
    draw_edge_Y(miny,maxy, maxx,minz, r,g,b);
    draw_edge_Y(miny,maxy, maxx,maxz, r,g,b);

    draw_edge_Z(minz,maxz, minx,miny, r,g,b);
    draw_edge_Z(minz,maxz, minx,maxy, r,g,b);
    draw_edge_Z(minz,maxz, maxx,miny, r,g,b);
    draw_edge_Z(minz,maxz, maxx,maxy, r,g,b);
}

void anim_shrink_cube(graph_animation_t *a)
{
    if (!st.inited){
        memset(&st, 0, sizeof(st));
        st.corner_idx = 0;
        st.size = 7;        // start plná kostka
        st.dir  = -1;       // nejdřív shrink
        st.rng  = HAL_GetTick();
        st.prev_r = 255; st.prev_g = 255; st.prev_b = 255; // sentinel (žádná předchozí)
        pick_random_color_distinct(&st.cr,&st.cg,&st.cb);
        st.inited = 1;
    }

    a->timer = HAL_GetTick() + 90;   // tempo
    memset(scrBuf, 0, sizeof(scrBuf));

    // aktivní roh
    const uint8_t cx = corners[st.corner_idx][0];
    const uint8_t cy = corners[st.corner_idx][1];
    const uint8_t cz = corners[st.corner_idx][2];

    // rozsah boxu z daného rohu (size 7..1 -> velikost 8..2)
    const int s = st.size;
    int minx = (cx==0) ? 0     : 7 - s;
    int maxx = (cx==0) ? s     : 7;
    int miny = (cy==0) ? 0     : 7 - s;
    int maxy = (cy==0) ? s     : 7;
    int minz = (cz==0) ? 0     : 7 - s;
    int maxz = (cz==0) ? s     : 7;

    // vykresli hrany aktuálního boxu
    draw_box_edges(minx,miny,minz, maxx,maxy,maxz, st.cr,st.cg,st.cb);

    // posun velikosti podle fáze
    st.size += st.dir;

    // přepínání fází a rohů:
    // min = 1 (=> 2x2x2), max = 7 (=> 8x8x8)
    if (st.dir < 0 && st.size <= 1){
        st.size = 1;
        st.dir  = +1;   // expand
    } else if (st.dir > 0 && st.size >= 7){
        st.size = 7;
        st.dir  = -1;   // další cyklus: shrink

        // ulož předchozí barvu a vyber novou, odlišnou
        st.prev_r = st.cr; st.prev_g = st.cg; st.prev_b = st.cb;
        pick_random_color_distinct(&st.cr,&st.cg,&st.cb);

        // přepni roh
        st.corner_idx = (uint8_t)((st.corner_idx + 1) & 7);
    }
}

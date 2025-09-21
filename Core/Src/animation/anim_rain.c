#include "anim_rain.h"
#include <stdint.h>
#include <string.h>

/* Externí buffer a čas */
extern Voxel_t scrBuf[8][8][8];
uint32_t HAL_GetTick(void);

/* ------------------------------ Stav ------------------------------ */
typedef struct {
    int8_t  x, y, z;       // 0..7
    uint8_t r, g, b;
    uint8_t alive;
} drop_t;

typedef struct {
    int8_t  x, y;          // střed splash
    uint8_t r, g, b;       // barva (odvozená z kapky)
    uint8_t age;           // 0..max_age
    uint8_t max_age;       // typicky 3..4
    uint8_t alive;
} splash_t;

static struct {
    uint32_t rng;
    uint8_t  inited;

    drop_t   drops[28];    // aktivní kapky
    splash_t spl[12];      // současné splash efekty

    uint8_t  thunder_ticks;     // 0=žádný záblesk, >0 právě probíhá flash
    uint8_t  thunder_cooldown;  // minimální odstup mezi blesky

} st;

/* ------------------------------ RNG ------------------------------ */
static inline uint8_t rr(void){
    st.rng = st.rng*1664525u + 1013904223u;
    return (uint8_t)(st.rng >> 24);
}

/* -------------------------- Spawn & helpers ----------------------- */
static void spawn_drop(void){
    for (int i=0;i<(int)(sizeof(st.drops)/sizeof(st.drops[0])); i++){
        if (!st.drops[i].alive){
            st.drops[i].x = rr() & 7;
            st.drops[i].y = rr() & 7;
            st.drops[i].z = 7;
            // azurově-modrá kapka s mírnou variací
            st.drops[i].r = 0;
            st.drops[i].g = (uint8_t)(120 + (rr() & 63));
            st.drops[i].b = (uint8_t)(180 + (rr() & 63));
            st.drops[i].alive = 1;
            return;
        }
    }
}

static void spawn_splash(int x, int y, uint8_t r, uint8_t g, uint8_t b){
    for (int i=0;i<(int)(sizeof(st.spl)/sizeof(st.spl[0])); i++){
        if (!st.spl[i].alive){
            st.spl[i].x = (int8_t)x;
            st.spl[i].y = (int8_t)y;
            st.spl[i].r = r;
            st.spl[i].g = g;
            st.spl[i].b = b;
            st.spl[i].age = 0;
            st.spl[i].max_age = 3 + (rr() % 2); // 3–4 kroky
            st.spl[i].alive = 1;
            return;
        }
    }
}

/* vykreslí splash „kroužek“ okolo středu, do z=0 a lehce i z=1 */
static void render_splash(const splash_t *s){
    if (!s->alive) return;
    int R = s->age;                // poloměr kruhu pro tento krok
    if (R == 0) R = 1;             // viditelný střed

    // slábnutí s věkem
    uint8_t f = (uint8_t)((255 * (s->max_age - s->age + 1)) / (s->max_age + 1));
    uint8_t r = (uint8_t)((s->r * f) / 255);
    uint8_t g = (uint8_t)((s->g * f) / 255);
    uint8_t b = (uint8_t)((s->b * f) / 255);

    for (int dx=-R; dx<=R; dx++){
        for (int dy=-R; dy<=R; dy++){
            int xx = s->x + dx;
            int yy = s->y + dy;
            if (xx<0||xx>7||yy<0||yy>7) continue;
            int d2 = dx*dx + dy*dy;
            // přibližný „prstenec“ (tolerance)
            if (d2 >= (R-1)*(R-1) && d2 <= (R+0)*(R+0)){
                // základ – zem (z=0)
                scrBuf[xx][yy][0].r = r;
                scrBuf[xx][yy][0].g = g;
                scrBuf[xx][yy][0].b = b;
                // lehký odskok – z=1, ztlumeně
                if (R <= 2 && 1 <= 7){
                    scrBuf[xx][yy][1].r = r>>2;
                    scrBuf[xx][yy][1].g = g>>2;
                    scrBuf[xx][yy][1].b = b>>2;
                }
            }
        }
    }
}

/* globální blesk – krátký záblesk celé scény */
static void render_thunder_flash(void){
    // projdeme buffer a přidáme bílý nádech
    for (int x=0;x<8;x++)
        for (int y=0;y<8;y++)
            for (int z=0;z<8;z++){
                // přidání bílého jasu (clamp)
                int r = scrBuf[x][y][z].r + 100;
                int g = scrBuf[x][y][z].g + 100;
                int b = scrBuf[x][y][z].b + 100;
                scrBuf[x][y][z].r = (uint8_t)(r>255?255:r);
                scrBuf[x][y][z].g = (uint8_t)(g>255?255:g);
                scrBuf[x][y][z].b = (uint8_t)(b>255?255:b);
            }
}

/* ------------------------------ Animace ------------------------------ */
void anim_rain(graph_animation_t *a)
{
    if (!st.inited){
        memset(&st, 0, sizeof(st));
        st.rng = HAL_GetTick();
        st.inited = 1;
    }

    a->timer = HAL_GetTick() + 80;   // svižný déšť (klidně +100 pro pomalejší)
    memset(scrBuf, 0, sizeof(scrBuf));

    /* Spawn deště – lehce náhodný, ale ne přespříliš */
    if ((rr() & 0x07) == 0) spawn_drop();     // ~1/8 ticků nová kapka
    if ((rr() & 0x0F) == 0) spawn_drop();     // občas druhá

    /* Bouřka: cooldown, pak náhodný blesk na 1–2 ticky */
    if (st.thunder_cooldown > 0) st.thunder_cooldown--;
    if (st.thunder_ticks == 0 && st.thunder_cooldown == 0){
        if ((rr() & 0x3F) == 0) {             // ~1/64 ticků
            st.thunder_ticks = 2 + (rr() & 1);// 1–2 snímky záblesku
            st.thunder_cooldown = 18 + (rr() & 7); // pauza do dalšího blesku
        }
    }

    /* Update & vykreslení kapek */
    for (int i=0;i<(int)(sizeof(st.drops)/sizeof(st.drops[0])); i++){
        drop_t *d = &st.drops[i];
        if (!d->alive) continue;

        // posun dolů
        if (d->z > 0) {
            d->z--;
        } else {
            // splash + zhasnout kapku
            spawn_splash(d->x, d->y, d->r, d->g, d->b);
            d->alive = 0;
            continue;
        }

        // vykresli kapku (hlava) + jemná šmouha nad ní
        scrBuf[d->x][d->y][d->z].r = d->r;
        scrBuf[d->x][d->y][d->z].g = d->g;
        scrBuf[d->x][d->y][d->z].b = d->b;

        if (d->z < 7) { // šmouha
            scrBuf[d->x][d->y][d->z+1].r = d->r >> 2;
            scrBuf[d->x][d->y][d->z+1].g = d->g >> 2;
            scrBuf[d->x][d->y][d->z+1].b = d->b >> 2;
        }
    }

    /* Update & vykreslení splash */
    for (int i=0;i<(int)(sizeof(st.spl)/sizeof(st.spl[0])); i++){
        splash_t *s = &st.spl[i];
        if (!s->alive) continue;

        render_splash(s);

        s->age++;
        if (s->age > s->max_age) s->alive = 0;
    }

    /* Blesk – přidej globální flash přes vše vykreslené */
    if (st.thunder_ticks > 0){
        render_thunder_flash();
        st.thunder_ticks--;
    }
}

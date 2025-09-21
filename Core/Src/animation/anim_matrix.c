#include "anim_matrix.h"
#include <stdint.h>
#include <string.h>

/* extern v projektu:
extern Voxel_t scrBuf[8][8][8];
uint32_t HAL_GetTick(void);
*/

typedef struct {
    int8_t  z_head;        // aktuální z hlavy (7..-L)
    uint8_t len;           // délka ocasu (1..6)
    uint8_t speed_div;     // jak často se posouvá (1=každý frame, 2=ob frame, ...)
    uint8_t tick;          // vnitřní čítač pro speed_div
    uint8_t cooldown;      // prodleva před dalším spuštěním (0=aktivní)
    uint8_t glitch_ticks;  // zbývající snímky glitch záblesku v tomto sloupci
} rain_col_t;

static struct {
    uint8_t  inited;
    uint32_t rng;
    rain_col_t col[8][8];
} rstate;

/* --- jednoduchý LCG RNG --- */
static inline uint8_t rr(void){
    rstate.rng = rstate.rng*1664525u + 1013904223u;
    return (uint8_t)(rstate.rng >> 24);
}

/* --- mírný globální fade (afterglow) --- */

// dříve: #define FADE_NUM 236  // ~0.922
#define FADE_NUM 208            // ~0.812 (rychlejší vyhasnutí)
#define FADE_DEN  256
static inline void fade_scene(void){
    for (int x=0;x<8;x++)
        for (int y=0;y<8;y++)
            for (int z=0;z<8;z++){
                scrBuf[x][y][z].r = (uint8_t)((scrBuf[x][y][z].r * FADE_NUM) / FADE_DEN);
                scrBuf[x][y][z].g = (uint8_t)((scrBuf[x][y][z].g * FADE_NUM) / FADE_DEN);
                scrBuf[x][y][z].b = (uint8_t)((scrBuf[x][y][z].b * FADE_NUM) / FADE_DEN);
            }
}

/* --- start jednoho sloupce --- */
static void spawn_column(int x,int y){
    rain_col_t *c = &rstate.col[x][y];
    c->z_head   = 7;
    c->len = 1 + (rr() & 2);                // 1..3
    c->speed_div= 1 + (rr() % 3);       // 1..3 (variace rychlosti)
    c->tick     = 0;
    c->cooldown = 0;
    c->glitch_ticks = 0;
}

/* --- inicializace všech sloupců --- */
static void init_rain(void){
    memset(&rstate, 0, sizeof(rstate));
    rstate.rng = HAL_GetTick();

    for (int x=0;x<8;x++){
        for (int y=0;y<8;y++){
            if ((rr() & 3)==0) spawn_column(x,y);
            else {
                rain_col_t *c = &rstate.col[x][y];
                c->z_head = -8;           // neaktivní
                c->len = 0;
                c->speed_div = 1 + (rr()%3);
                c->tick = 0;
                c->cooldown = (uint8_t)(5 + (rr() & 15)); // 5..20 ticků
                c->glitch_ticks = 0;
            }
        }
    }
    rstate.inited = 1;
}

/* --- přičti barvu s ořezem --- */
static inline void add_voxel_g(int x,int y,int z, uint8_t r,uint8_t g,uint8_t b){
    if ((unsigned)x>7||(unsigned)y>7||(unsigned)z>7) return;
    int R = scrBuf[x][y][z].r + r;
    int G = scrBuf[x][y][z].g + g;
    int B = scrBuf[x][y][z].b + b;
    scrBuf[x][y][z].r = (R>255)?255:(uint8_t)R;
    scrBuf[x][y][z].g = (G>255)?255:(uint8_t)G;
    scrBuf[x][y][z].b = (B>255)?255:(uint8_t)B;
}

/* --- hlavní animace --- */
void anim_matrix(graph_animation_t *a)
{
    if (!rstate.inited) init_rain();

    a->timer = HAL_GetTick() + 80;   // ~12.5 FPS
    fade_scene();

    for (int x=0;x<8;x++){
        for (int y=0;y<8;y++){
            rain_col_t *c = &rstate.col[x][y];

            /* Glitch – občasný start, krátký (1–2 snímky) jasný flash v celém sloupci */
            if (c->glitch_ticks == 0 && c->cooldown == 0) {
                if ((rr() & 0x3F) == 0) {           // ~1/64 šance per tick
                    c->glitch_ticks = 1 + (rr() & 1); // 1–2 snímky
                }
            }
            if (c->glitch_ticks) {
                // silný zelený flash napříč Z
                for (int z=0; z<8; ++z) add_voxel_g(x,y,z, 0, 255, 0);
                c->glitch_ticks--;
            }

            /* cooldown: sloupec odpočívá a občas se znovu spustí */
            if (c->cooldown){
                c->cooldown--;
                if (c->cooldown==0 && (rr() & 1)==0) spawn_column(x,y);
                continue;
            }

            /* aktivní sloupec – posun podle rychlosti */
            if (++c->tick >= c->speed_div){
                c->tick = 0;
                c->z_head--;   // padá dolů (7 -> -L)
            }

            /* vykresli hlavu (neon zelená) */
            int zh = c->z_head;
            if ((unsigned)zh <= 7){
                add_voxel_g(x,y,zh,  10, 255, 20);  // méně bílé, více čistě zelené
                // volitelná jemná „koróna“ nad hlavou:
                if (zh < 7) add_voxel_g(x,y,zh+1, 0, 80, 0);
            }

            /* ocas: zh+1 .. zh+len (čím dál, tím slabší, čistě zelené odstíny) */
            for (int i=1; i<=c->len; ++i){
                int zt = zh + i;
                if ((unsigned)zt > 7) break;
                // lineární pokles jasu
                // dříve (lineární):
                // uint8_t g = (uint8_t)(220 - (i-1)*(140 / (c->len?c->len:1)));

                // nově (kvadraticky a s nižším stropem):
                int L = (c->len ? c->len : 1);
                int num = (i*i);              // 1,4,9...
                int den = (L*L);              // 1,4,9...
                uint8_t g = (uint8_t)(180 - (num * 160) / den); // 180 .. ~20
                if (g > 180) g = 180;         // clamp (pro i=1, L=1)
                if (g < 10)  g = 10;
                add_voxel_g(x,y,zt, 0, g, 0);
            }

            /* WATERFALL symbol: náhodná extra „kapka“ uvnitř ocasu (jasnější záblesk) */
            if ((rr() & 0x0F) == 0) {                  // ~1/8 ticků per sloupec
                int span = (c->len ? c->len : 1) + 1;  // rozsah ocasu
                int wz = zh + 1 + (rr() % span);       // někde v ocasu/za hlavou
                if ((unsigned)wz <= 7) {
                    add_voxel_g(x,y,wz, 0, 255, 0);    // jasný zelený „symbol“
                    if (wz+1 <= 7) add_voxel_g(x,y,wz+1, 0, 60, 0); // lehká stopa
                }
            }

            /* když vyjel z kostky, dej pauzu a připrav nové parametry */
            if (c->z_head < -((int)c->len)){
                c->cooldown   = (uint8_t)(6 + (rr() & 31)); // 6..37 ticků pauza
                c->speed_div  = 1 + (rr() % 3);
                c->len        = 2 + (rr() & 3);
                c->glitch_ticks = 0;
            }
        }
    }
}

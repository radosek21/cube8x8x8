#include "anim_sine_wave.h"
#include <stdint.h>
#include <string.h>

/* extern:
extern Voxel_t scrBuf[8][8][8];
uint32_t HAL_GetTick(void);
*/

/* --- sin(2π*n/32) * 127 (int8_t) --- */
static const int8_t sin32[32] = {
     0,  25,  49,  71,  90, 106, 117, 125,
   127, 125, 117, 106,  90,  71,  49,  25,
     0, -25, -49, -71, -90,-106,-117,-125,
  -127,-125,-117,-106, -90, -71, -49, -25
};
static inline int isine32(uint8_t a) { return sin32[a & 31]; }

/* --- paleta 8 vrstev: z=0..7 (violet -> red) --- */
static const Voxel_t zpal[8] = {
    {148,   0, 211},  /* 0 Violet  */
    { 75,   0, 130},  /* 1 Indigo  */
    {  0,   0, 255},  /* 2 Blue    */
    {  0, 255, 255},  /* 3 Cyan    */
    {  0, 255,   0},  /* 4 Green   */
    {255, 255,   0},  /* 5 Yellow  */
    {255, 165,   0},  /* 6 Orange  */
    {255,   0,   0}   /* 7 Red     */
};

/* --- stav --- */
static uint8_t phase = 0;

/* parametry vlny (lze ladit) */
static const int centerZ = 3;     /* střed (0..7) */
static const int ampZ    = 4;     /* amplituda */
static const uint8_t kx  = 4;     /* prostorová frekvence po X */
static const uint8_t dph = 2;     /* rychlost posuvu fáze */

/* pomocná: zapsat voxel s barvou (případně tlumenou) bezpečně */
static inline void put_voxel_dim(int x,int y,int z, Voxel_t c, int shift)
{
    if ((unsigned)x>7 || (unsigned)y>7 || (unsigned)z>7) return;
    if (shift <= 0) {
        scrBuf[x][y][z] = c;
    } else {
        scrBuf[x][y][z].r = (uint8_t)(c.r >> shift);
        scrBuf[x][y][z].g = (uint8_t)(c.g >> shift);
        scrBuf[x][y][z].b = (uint8_t)(c.b >> shift);
    }
}

/* --- hlavní animace --- */
void anim_sine_wave(graph_animation_t *a)
{
    if (a->reset) {
      phase = 0;
      a->reset = 0;
    }

    a->timer = HAL_GetTick() + 80;        /* tempo */
    memset(scrBuf, 0, sizeof(scrBuf));    /* čistý frame */

    for (int x = 0; x < 8; x++) {
        /* Z(x) = center + amp * sin( kx*x + phase ) */
        uint8_t ax = (uint8_t)(x * kx + phase);
        int s = isine32(ax);                  /* -127..127 */
        int z = centerZ + (ampZ * s) / 127;   /* celé číslo */
        if (z < 0) z = 0; else if (z > 7) z = 7;

        /* barvy pro hlavní vrstvu a sousední vrstvy */
        Voxel_t c_main = zpal[z];

        /* volitelně: sousední vrstvy použijeme se STEJNOU barvou jako hlavní,
           jen ztlumenou (čistý „halo“ efekt). Pokud chceš, lze místo toho
           vzít paletu zpal[z-1]/zpal[z+1]. */
        for (int y = 0; y < 8; y++) {
            /* hlavní linka */
            put_voxel_dim(x, y, z, c_main, 0);

            /* fade kolem vlny: z-1 a z+1 s menším jasem */
            if (z > 0)     put_voxel_dim(x, y, z-1, c_main, 3);  /* ~25 % */
            if (z < 7)     put_voxel_dim(x, y, z+1, c_main, 3);  /* ~25 % */
        }
    }

    phase += dph;  /* posun vlny v čase */
}

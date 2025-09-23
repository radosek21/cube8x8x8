/*
 * anim_pong.c  -- 3D Pong s file-static stavem
 */

#include "anim_ping_pong.h"
#include <stdint.h>
#include <string.h>

/* ---------- Stav animace ---------- */
typedef struct {
    int x_fp, y_fp, z_fp;   /* fixed-point 8.8 pozice */
    int vx, vy, vz;         /* rychlosti v 1/256 voxelu/tick */
    int paddleL_y, paddleL_z;
    int paddleR_y, paddleR_z;
    uint32_t rng;           /* LCG seed */
    int flashL_ticks;       /* krátký záblesk levé pálky */
    int flashR_ticks;       /* krátký záblesk pravé pálky */
} pong_state_t;

static pong_state_t st;

/* ---------- Utility ---------- */
static inline int step_toward(int cur, int target) {
    if (cur < target) return cur + 1;
    if (cur > target) return cur - 1;
    return cur;
}

static inline int pong_next_rnd(void) {
    st.rng = st.rng * 1664525u + 1013904223u;
    return (int)((st.rng >> 16) & 0xFF);
}

/* kreslení 2x2x2 míčku */
static void draw_ball(int x_vox,int y_vox,int z_vox,uint8_t r,uint8_t g,uint8_t b) {
    for (int dx=0; dx<2; ++dx) {
        int x = x_vox + dx;
        if (x<0 || x>7) continue;
        for (int dy=0; dy<2; ++dy) {
            int y = y_vox + dy;
            if (y<0 || y>7) continue;
            for (int dz=0; dz<2; ++dz) {
                int z = z_vox + dz;
                if (z<0 || z>7) continue;
                scrBuf[x][y][z].r = r;
                scrBuf[x][y][z].g = g;
                scrBuf[x][y][z].b = b;
            }
        }
    }
}

/* pálka vlevo (x=0) */
static void draw_paddle_left(int cy,int cz,uint8_t r,uint8_t g,uint8_t b) {
    int y0 = cy - 1;
    int z0 = cz - 1;
    for (int yy=0; yy<3; ++yy) {
        int y = y0 + yy;
        if (y<0 || y>7) continue;
        for (int zz=0; zz<3; ++zz) {
            int z = z0 + zz;
            if (z<0 || z>7) continue;
            scrBuf[0][y][z].r = r;
            scrBuf[0][y][z].g = g;
            scrBuf[0][y][z].b = b;
        }
    }
}

/* pálka vpravo (x=7) */
static void draw_paddle_right(int cy,int cz,uint8_t r,uint8_t g,uint8_t b) {
    int y0 = cy - 1;
    int z0 = cz - 1;
    for (int yy=0; yy<3; ++yy) {
        int y = y0 + yy;
        if (y<0 || y>7) continue;
        for (int zz=0; zz<3; ++zz) {
            int z = z0 + zz;
            if (z<0 || z>7) continue;
            scrBuf[7][y][z].r = r;
            scrBuf[7][y][z].g = g;
            scrBuf[7][y][z].b = b;
        }
    }
}


/* ... utility funkce step_toward a pong_next_rnd zůstávají stejné ... */

/* ---------- Hlavní animace ---------- */
void anim_ping_pong(graph_animation_t *a)
{
    if (!a) return;

    if (a->reset) {
        st.x_fp = 1*256;
        st.y_fp = 3*256;
        st.z_fp = 3*256;
        st.vx = 200; st.vy = 60; st.vz = 40;
        st.paddleL_y = 3; st.paddleL_z = 3;
        st.paddleR_y = 3; st.paddleR_z = 3;
        st.rng = (uint32_t)HAL_GetTick();
        st.flashL_ticks = 0;
        st.flashR_ticks = 0;
        a->reset = 0;
    }

    a->timer = HAL_GetTick() + 100; // krok 100ms
    memset(scrBuf, 0, sizeof(scrBuf));

    // Pohyb míčku
    st.x_fp += st.vx;
    st.y_fp += st.vy;
    st.z_fp += st.vz;

    if (st.y_fp < 0) { st.y_fp = 0; st.vy = -st.vy; }
    if (st.y_fp > 7*256) { st.y_fp = 7*256; st.vy = -st.vy; }
    if (st.z_fp < 0) { st.z_fp = 0; st.vz = -st.vz; }
    if (st.z_fp > 7*256) { st.z_fp = 7*256; st.vz = -st.vz; }

    int bx = st.x_fp >> 8;
    int by = st.y_fp >> 8;
    int bz = st.z_fp >> 8;

    // Pálky krokově
    st.paddleL_y = step_toward(st.paddleL_y, by);
    st.paddleL_z = step_toward(st.paddleL_z, bz);
    st.paddleR_y = step_toward(st.paddleR_y, by);
    st.paddleR_z = step_toward(st.paddleR_z, bz);

    // Levá stěna / pálka
    if (st.x_fp <= 0) {
        int hit = 0;
        for (int yy=-1; yy<=1 && !hit; ++yy)
            for (int zz=-1; zz<=1; ++zz)
                if ((st.paddleL_y+yy == by || st.paddleL_y+yy == by+1) &&
                    (st.paddleL_z+zz == bz || st.paddleL_z+zz == bz+1)) hit=1;

        st.vx = -st.vx; st.x_fp = 2;
        if (hit) {
            st.vy += ((pong_next_rnd() & 0x3F)-32)*2;
            st.vz += ((pong_next_rnd() & 0x3F)-32)*2;
            st.flashL_ticks = 4; // jen levá pálka
        }
    }

    // Pravá stěna / pálka
    if (st.x_fp >= 7*256) {
        int hit = 0;
        for (int yy=-1; yy<=1 && !hit; ++yy)
            for (int zz=-1; zz<=1; ++zz)
                if ((st.paddleR_y+yy == by || st.paddleR_y+yy == by+1) &&
                    (st.paddleR_z+zz == bz || st.paddleR_z+zz == bz+1)) hit=1;

        st.vx = -st.vx; st.x_fp = 7*256-2;
        if (hit) {
            st.vy += ((pong_next_rnd() & 0x3F)-32)*2;
            st.vz += ((pong_next_rnd() & 0x3F)-32)*2;
            st.flashR_ticks = 4; // jen pravá pálka
        }
    }

    // Omezení rychlostí
    if (st.vx>1024) st.vx=1024;
    if (st.vx<-1024) st.vx=-1024;
    if (st.vy>1024) st.vy=1024;
    if (st.vy<-1024) st.vy=-1024;
    if (st.vz>1024) st.vz=1024;
    if (st.vz<-1024) st.vz=-1024;

    // Kreslení pálky
    draw_paddle_left(st.paddleL_y, st.paddleL_z, 180 + (st.flashL_ticks>0?80:0), 0, 0);
    draw_paddle_right(st.paddleR_y, st.paddleR_z, 0, 0, 180 + (st.flashR_ticks>0?80:0));

    // Míček
    draw_ball(bx, by, bz,
              (st.flashL_ticks>0 || st.flashR_ticks>0) ? 255 : 220,
              (st.flashL_ticks>0 || st.flashR_ticks>0) ? 200 : 220,
              (st.flashL_ticks>0 || st.flashR_ticks>0) ? 80  : 220);

    // Sniž flash ticks
    if (st.flashL_ticks>0) st.flashL_ticks--;
    if (st.flashR_ticks>0) st.flashR_ticks--;

    st.vy = (st.vy*97)/100;
    st.vz = (st.vz*97)/100;
}

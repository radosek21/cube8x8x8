#include "anim_pyre.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

extern Voxel_t scrBuf[8][8][8];

/* === ladění === */
#define FRAME_MS     50
#define SEED_CHANCE  3
#define COOL_BASE    2
#define COOL_STEP    1
#define COOL_NOISE   2

#define W_CENTER     6
#define W_AXIAL      1
#define RADIUS_MAX   2.6f
#define CONE_POWER   2.0f
/* ============= */

static inline void pset(int x,int y,int z, Voxel_t c){
    if((unsigned)x<8 && (unsigned)y<8 && (unsigned)z<8)
        scrBuf[x][7 - y][z] = c;   // Y je převrácená
}

/* heat (0..255) -> RGB oheň (víc žluté) */
static inline Voxel_t heat2rgb(uint8_t h){
    uint8_t r=0,g=0,b=0;
    if(h < 64){ r = h<<2; }
    else if(h < 128){ r=255; g=(h-64)<<2; }
    else if(h < 192){ r=255; g=200 + ((h-128)<<1); }
    else{ r=255; g=255; b=(h-192)<<2; }
    return (Voxel_t){r,g,b};
}

static inline uint8_t get(const uint8_t *H, int x,int y,int z){
    if((unsigned)x<8 && (unsigned)y<8 && (unsigned)z<8) return H[(z*8 + y)*8 + x];
    return 0;
}
static inline void set(uint8_t *H, int x,int y,int z, uint8_t v){
    if((unsigned)x<8 && (unsigned)y<8 && (unsigned)z<8) H[(z*8 + y)*8 + x] = v;
}

void anim_pyre(graph_animation_t *a)
{
    static uint8_t heat[8*8*8];
    static uint8_t nextH[8*8*8];

    if(a->reset){
        memset(heat, 0, sizeof(heat));
        memset(nextH, 0, sizeof(nextH));
        a->reset = 0;                 // nulujeme uvnitř
    }

    a->timer = HAL_GetTick() + FRAME_MS;

    const float cx = 3.5f, cy = 3.5f;

    /* 1) osévat spodek (z=0) „palivem“ – konfinované do středu */
    for(int y=0;y<8;y++){
        for(int x=0;x<8;x++){
            float dx = x - cx, dy = y - cy;
            float r2 = dx*dx + dy*dy;
            float conf = 1.0f - (r2 / (RADIUS_MAX*RADIUS_MAX));
            if(conf < 0.f) conf = 0.f;
            conf *= conf; // ostřejší kužel
            uint8_t base = (uint8_t)((rand() & 0x3F) * conf);
            if((rand()%SEED_CHANCE)==0){
                uint8_t spike = (uint8_t)(160 + (rand()&0x3F));
                base = (uint8_t)((base + (uint8_t)(spike*conf))>>1);
            }
            uint8_t prev = get(heat,x,y,0);
            set(heat,x,y,0,(uint8_t)((prev + base)>>1));
        }
    }

    /* 2) výškové šíření + ochlazování */
    for(int z=7; z>=1; --z){
        const uint8_t cool = (uint8_t)(COOL_BASE + COOL_STEP*z + (rand()%COOL_NOISE));
        for(int y=0;y<8;y++){
            for(int x=0;x<8;x++){
                uint16_t sum=0, w=0;
                sum += W_CENTER*get(heat,x,y,z-1); w+=W_CENTER;
                sum += W_AXIAL*get(heat,x-1,y,z-1); w+=W_AXIAL;
                sum += W_AXIAL*get(heat,x+1,y,z-1); w+=W_AXIAL;
                sum += W_AXIAL*get(heat,x,y-1,z-1); w+=W_AXIAL;
                sum += W_AXIAL*get(heat,x,y+1,z-1); w+=W_AXIAL;

                uint8_t val = (uint8_t)(sum/(w?w:1));
                int cooled = (int)val - cool;
                if(cooled<0) cooled=0;
                set(nextH,x,y,z,(uint8_t)cooled);
            }
        }
    }
    // z=0 mírně chladnout
    for(int y=0;y<8;y++){
        for(int x=0;x<8;x++){
            int v = (int)get(heat,x,y,0) - (COOL_BASE>>1);
            if(v<0) v=0;
            set(nextH,x,y,0,(uint8_t)v);
        }
    }

    /* 3) swap */
    memcpy(heat, nextH, sizeof(heat));

    /* 4) vykreslení */
    memset(scrBuf, 0, sizeof scrBuf);

    // --- polena jako kříž (z=0), dva trámy přes sebe, široké 2 voxely ---
    const Voxel_t log_col  = (Voxel_t){60,30,10};
    const Voxel_t glow_col = (Voxel_t){200,80,20};

    // vodorovný trám (po ose X) přes střed (y=3..4)
    for(int x=1; x<=6; ++x){
        pset(x,3,0,log_col);
        pset(x,4,0,log_col);
    }
    // svislý trám (po ose Y) přes střed (x=3..4)
    for(int y=1; y<=6; ++y){
        pset(3,y,0,log_col);
        pset(4,y,0,log_col);
    }

    // občasné žhavé místečko na kříži
    if((rand()%18)==0){
        int pick = rand()%2; // 0: vodorovný, 1: svislý
        if(pick==0){
            int rx = 1 + (rand()%6);
            int ry = 3 + (rand()%2);
            pset(rx,ry,0,glow_col);
        }else{
            int rx = 3 + (rand()%2);
            int ry = 1 + (rand()%6);
            pset(rx,ry,0,glow_col);
        }
    }

    // --- plamen + halo ve vrstvách z=1..7 ---
    for(int z=1; z<8; ++z){
        for(int y=0; y<8; ++y){
            for(int x=0; x<8; ++x){
                uint8_t h = get(heat,x,y,z);
                if(h){
                    pset(x,y,z, heat2rgb(h));
                }else{
                    // decentní oranž/žluté halo u krajů plamene
                    int n = get(heat,x-1,y,z)+get(heat,x+1,y,z)+
                            get(heat,x,y-1,z)+get(heat,x,y+1,z);
                    if(n > 220){
                        pset(x,y,z,(Voxel_t){220,140,40}); // oranžová
                    }else if(n > 120){
                        pset(x,y,z,(Voxel_t){240,200,60}); // žlutá
                    }
                }
            }
        }
    }

    // jiskry
    if((rand() & 7)==0){
        int sx=rand()&7, sy=rand()&7, sz=2+(rand()%5);
        if(get(heat,sx,sy,sz)>170)
            pset(sx,sy,sz+1<8?sz+1:sz,(Voxel_t){255,255,180});
    }
}

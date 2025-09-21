#include "anim_test.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern Voxel_t scrBuf[8][8][8];

static inline void fade_all(uint8_t dec){
    for(int z=0;z<8;z++) for(int y=0;y<8;y++) for(int x=0;x<8;x++){
        Voxel_t *v=&scrBuf[z][y][x];
        v->r = (v->r>dec)? v->r-dec : 0;
        v->g = (v->g>dec)? v->g-dec : 0;
        v->b = (v->b>dec)? v->b-dec : 0;
    }
}

static inline Voxel_t hsv(uint16_t h, uint8_t s, uint8_t v){
    if(s==0) return (Voxel_t){v,v,v};
    uint8_t region = (h/60)%6;
    uint16_t f = (h%60)*255/60;
    uint16_t p = (uint16_t)v*(255 - s)/255;
    uint16_t q = (uint16_t)v*(255 - (s*f)/255)/255;
    uint16_t t = (uint16_t)v*(255 - (s*(255 - f))/255)/255;
    switch(region){
        case 0: return (Voxel_t){v, t, p};
        case 1: return (Voxel_t){q, v, p};
        case 2: return (Voxel_t){p, v, t};
        case 3: return (Voxel_t){p, q, v};
        case 4: return (Voxel_t){t, p, v};
        default:return (Voxel_t){v, p, q};
    }
}
static inline Voxel_t random_color(void){
    return hsv(rand()%360, 220, 220);
}

void anim_test(graph_animation_t *a)
{
    // Q8.8 fixed-point: 1.0 voxel == 256
    static int inited=0;
    static int32_t px,py,pz;     // pozice min-rohu
    static int32_t vx,vy,vz;     // rychlost
    static int s;                // hrana krychle
    static Voxel_t col;

    // === laditelné parametry rychlosti ===
    const int32_t VMIN = 12;   // ~0.047 vox/frame
    const int32_t VMAX = 48;   // ~0.188 vox/frame
    // =====================================

    if(!inited){
        inited=1;
        s  = 3;
        px = (rand()%(8 - s + 1))<<8;
        py = (rand()%(8 - s + 1))<<8;
        pz = (rand()%(8 - s + 1))<<8;

        // pomalejší start: ±32 (≈ 0.125 vox/frame)
        vx = ((rand()&1)? +32 : -32);
        vy = ((rand()&1)? +32 : -32);
        vz = ((rand()&1)? +32 : -32);

        col = (a->v.r||a->v.g||a->v.b)? a->v : random_color();
    }

    // vysoký framerate pro plynulost
    a->timer = HAL_GetTick() + 30;

    // jemné toulání – menší jitter a o něco vzácněji
    if((rand()%12)==0){
        int axis = rand()%3;
        int d = (rand()%5) - 2; // -2..+2
        if(axis==0) vx += d;
        else if(axis==1) vy += d;
        else vz += d;

        // clamp + nikdy nezůstat stát
        if(vx>+VMAX) vx=+VMAX; if(vx<-VMAX) vx=-VMAX;
        if(vy>+VMAX) vy=+VMAX; if(vy<-VMAX) vy=-VMAX;
        if(vz>+VMAX) vz=+VMAX; if(vz<-VMAX) vz=-VMAX;
        if(vx==0 && vy==0 && vz==0){
            int sign = (rand()&1)? +1 : -1;
            int pick = rand()%3;
            if(pick==0) vx = sign*VMIN;
            else if(pick==1) vy = sign*VMIN;
            else vz = sign*VMIN;
        }
    }

    // posun
    px += vx; py += vy; pz += vz;

    // hranice v Q8.8
    const int32_t MIN = 0;
    const int32_t MAX = (8 - s) << 8;

    int bounced = 0;
    if(px < MIN){ px = MIN; vx = -vx; bounced=1; }
    if(py < MIN){ py = MIN; vy = -vy; bounced=1; }
    if(pz < MIN){ pz = MIN; vz = -vz; bounced=1; }
    if(px > MAX){ px = MAX; vx = -vx; bounced=1; }
    if(py > MAX){ py = MAX; vy = -vy; bounced=1; }
    if(pz > MAX){ pz = MAX; vz = -vz; bounced=1; }
    if(bounced){
        col = random_color();
        // drobná asymetrie po odrazu, ale menší než dřív
        int jitter = ((rand()%7)-3); // -3..+3
        vx += jitter; vy -= jitter;
        // držet v limitech
        if(vx>+VMAX) vx=+VMAX; if(vx<-VMAX) vx=-VMAX;
        if(vy>+VMAX) vy=+VMAX; if(vy<-VMAX) vy=-VMAX;
        if(vz>+VMAX) vz=+VMAX; if(vz<-VMAX) vz=-VMAX;
    }

    // jemný fade pro hladký dojem
    fade_all(10);

    // vykreslení
    int x = px >> 8, y = py >> 8, z = pz >> 8;
    for(int zz=0; zz<s; ++zz)
        for(int yy=0; yy<s; ++yy)
            for(int xx=0; xx<s; ++xx)
                scrBuf[z+zz][y+yy][x+xx] = col;
}

#include "anim_explosion.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

extern Voxel_t scrBuf[8][8][8];

/* Jednoduché HSV→RGB (h:0..359, s/v:0..255) */
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

void anim_explosion(graph_animation_t *a)
{
    static float R;          // aktuální poloměr v „voxelových“ jednotkách
    static float dR;         // rychlost rozpínání (vox/frame)
    static int hueBase;      // základní posun barev

    if(a->reset){
        a->reset = 0;
        R = 0.0f;
        dR = 0.25f;          // zrychli/zpomal dle chuti
        hueBase = 0;
    }

    // plynulost (cca 20 fps)
    a->timer = HAL_GetTick() + 50;

    // vyčisti obraz
    memset(scrBuf, 0, sizeof scrBuf);

    // střed (počítáme ke středům voxelů)
    const float cx = 3.5f, cy = 3.5f, cz = 3.5f;

    // tolerance tloušťky skořápky (menší -> tenčí prstenec)
    const float thickness = 0.45f;

    // pro každý voxel spočti vzdálenost od středu a srovnej s R
    for(int z=0; z<8; ++z){
        for(int y=0; y<8; ++y){
            for(int x=0; x<8; ++x){
                float dx = x - cx;
                float dy = y - cy;
                float dz = z - cz;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);

                // je voxel poblíž vlnové fronty?
                if (fabsf(dist - R) <= thickness){
                    // barva: jemně závislá na dist + globální posun
                    int hue = (hueBase + (int)(dist*40.0f)) % 360;
                    Voxel_t c = (a->v.r||a->v.g||a->v.b) ? a->v : hsv(hue, 220, 230);
                    scrBuf[z][y][x] = c;
                }
            }
        }
    }

    // update poloměru a barev
    R += dR;
    hueBase = (hueBase + 3) % 360;

    // maximální možný poloměr do rohu ~ sqrt(3)*3.5 ≈ 6.06
    if (R > 6.2f){
        R = 0.0f;
        // volitelně malý náhodný skok v barvě
        hueBase = (hueBase + (rand()%60)) % 360;
    }
}

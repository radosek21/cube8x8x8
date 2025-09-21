#include "anim_test.h"
#include <stdint.h>
#include <string.h>

#define FW_GROUND_KILL_Z   2   // jiskry zmizí, jakmile klesnou na z<=2 (ve vzduchu)
#define FW_AIR_DRAG_NUM    245 // volitelné mírné tlumení rychlosti (≈0.96)
#define FW_AIR_DRAG_DEN    256

/* --------------------- Pomocné funkce --------------------- */
static inline uint8_t clamp8i(int v){ return (v<0)?0: (v>255)?255:(uint8_t)v; }

/* „Additivní“ zápis s ořezem (hezké pro částečné překrytí) */
static inline void add_voxel(int x,int y,int z, uint8_t r,uint8_t g,uint8_t b){
    if((unsigned)x>7||(unsigned)y>7||(unsigned)z>7) return;
    int R = scrBuf[x][y][z].r + r;
    int G = scrBuf[x][y][z].g + g;
    int B = scrBuf[x][y][z].b + b;
    scrBuf[x][y][z].r = (R>255)?255:(uint8_t)R;
    scrBuf[x][y][z].g = (G>255)?255:(uint8_t)G;
    scrBuf[x][y][z].b = (B>255)?255:(uint8_t)B;
}

/* ----------------------- Částice -------------------------- */
typedef struct {
    int x_fp, y_fp, z_fp;      /* 8.8 fixed-point pozice (voxel*256) */
    int vx, vy, vz;            /* rychlosti v 1/256 voxel/tick */
    uint8_t r,g,b;             /* barva (aktuální jas) */
    uint8_t life;              /* zbývající život (ticků) */
    uint8_t alive;
} spark_t;

typedef struct {
    int x_fp, y_fp, z_fp;      /* pozice rakety (8.8) */
    int vx, vy, vz;            /* mírný drift + stoupání */
    uint8_t r,g,b;             /* barva efektu (pro stopu/explosi) */
    uint8_t fuse;              /* pojistka – po vyhoření exploduje */
    uint8_t alive;
} rocket_t;

/* ----------------------- Stav scény ----------------------- */
static struct {
    uint32_t rng;
    uint8_t  inited;

    rocket_t rockets[3];       /* několik současných raket */
    spark_t  sparks[64];       /* jiskry po explosi */

} fw;

/* RNG (LCG, rychlý) */
static inline uint8_t rr(void){
    fw.rng = fw.rng*1664525u + 1013904223u;
    return (uint8_t)(fw.rng>>24);
}

/* Náhodná jasná barva (RGB bez mdlé šedi) */
static void rand_bright_color(uint8_t *r, uint8_t *g, uint8_t *b){
    uint8_t c1 = 128 + (rr() & 127);       /* silná složka */
    uint8_t c2 =  64 + (rr() & 127);       /* střední */
    uint8_t c3 =  10 + (rr() & 63);        /* slabá */
    /* náhodné promíchání pořadí RGB */
    switch(rr()%6){
        case 0: *r=c1; *g=c2; *b=c3; break;
        case 1: *r=c1; *g=c3; *b=c2; break;
        case 2: *r=c2; *g=c1; *b=c3; break;
        case 3: *r=c2; *g=c3; *b=c1; break;
        case 4: *r=c3; *g=c1; *b=c2; break;
        default:*r=c3; *g=c2; *b=c1; break;
    }
}

/* Spawn rakety ze země (z=0) s lehkým driftem a barvou */
static void spawn_rocket(void){
    for(int i=0;i<(int)(sizeof(fw.rockets)/sizeof(fw.rockets[0])); ++i){
        rocket_t *r = &fw.rockets[i];
        if(!r->alive){
            r->x_fp = (rr()&7)*256;
            r->y_fp = (rr()&7)*256;
            r->z_fp = 0;
            /* rychlosti: stoupání + drobný drift v X/Y */
            r->vx = ((int)((int8_t)(rr()%7 - 3))) * 20;   /* -60..+60 */
            r->vy = ((int)((int8_t)(rr()%7 - 3))) * 20;
            r->vz = 180 + (rr() & 63);                    /* 180..243 (≈0.7..0.95 vox/tick) */
            rand_bright_color(&r->r,&r->g,&r->b);
            r->fuse = 8 + (rr() & 7);                     /* 8–15 ticků do explose (nebo výška) */
            r->alive = 1;
            return;
        }
    }
}

/* Vytvoř jiskry z jedné rakety */
static void explode_rocket(const rocket_t *r){
    /* kolik jisker — malá scéna, 18–28 je tak akorát */
    int count = 18 + (rr() % 11);
    for(int i=0;i<(int)(sizeof(fw.sparks)/sizeof(fw.sparks[0])) && count>0; ++i){
        spark_t *s = &fw.sparks[i];
        if(!s->alive){
            s->x_fp = r->x_fp; s->y_fp = r->y_fp; s->z_fp = r->z_fp;
            /* náhodný směr (přibližně kulově), různá rychlost */
            int ax = ((int)((int8_t)(rr() - 128)));   /* -128..127 */
            int ay = ((int)((int8_t)(rr() - 128)));
            int az = ((int)((int8_t)(rr() - 64)));    /* lehce preferovat nahoru */
            /* normalizace „zhruba“: zmenšíme vektor, aby nebyl přestřelený */
            ax >>= 1; ay >>= 1; az >>= 1;
            /* škála rychlosti (1/256 vox/tick) */
            int speed = 150 + (rr() & 127);           /* 150..277 */
            s->vx = (ax * speed) / 180;
            s->vy = (ay * speed) / 180;
            s->vz = (az * speed) / 180;

            /* barva: kolem barvy rakety s troškou náhody */
            int dr = (int)r->r + ((int)((int8_t)(rr()&63)) - 31);
            int dg = (int)r->g + ((int)((int8_t)(rr()&63)) - 31);
            int db = (int)r->b + ((int)((int8_t)(rr()&63)) - 31);
            s->r = clamp8i(dr); s->g = clamp8i(dg); s->b = clamp8i(db);

            s->life = 8 + (rr() & 5);   // 8–13 ticků života (dřív zmizí ve vzduchu)
            s->alive = 1;
            --count;
        }
    }
}

/* ----------------------- Hlavní animace ----------------------- */
void anim_firework(graph_animation_t *a)
{
    if(!fw.inited){
        memset(&fw,0,sizeof(fw));
        fw.rng = HAL_GetTick();
        fw.inited = 1;
    }

    /* rychlost animace — 70–100 ms vypadá dobře */
    a->timer = HAL_GetTick() + 80;

    /* ostré vykreslení – čistíme každý frame */
    memset(scrBuf, 0, sizeof(scrBuf));

    /* náhodně spouštět rakety (omezovat počet) */
    if((rr() & 0x07)==0){           /* ~1/8 ticků zkusit */
        int alive=0; for(int i=0;i<(int)(sizeof(fw.rockets)/sizeof(fw.rockets[0]));++i) alive+=fw.rockets[i].alive;
        if(alive < (int)(sizeof(fw.rockets)/sizeof(fw.rockets[0]))) spawn_rocket();
    }

    /* --- Update raket --- */
    for(int i=0;i<(int)(sizeof(fw.rockets)/sizeof(fw.rockets[0])); ++i){
        rocket_t *r = &fw.rockets[i];
        if(!r->alive) continue;

        /* pohyb */
        r->x_fp += r->vx;
        r->y_fp += r->vy;
        r->z_fp += r->vz;

        /* malá turbulence: drobná změna driftu */
        if((rr() & 3)==0){ r->vx += ((int8_t)(rr()%5 - 2)); r->vy += ((int8_t)(rr()%5 - 2)); }

        /* stopa (z-1) – tlumená barva */
        int x = r->x_fp>>8, y = r->y_fp>>8, z = r->z_fp>>8;
        add_voxel(x, y, z, r->r, r->g, r->b);
        if(z>0) add_voxel(x, y, z-1, r->r>>2, r->g>>2, r->b>>2);

        /* pojistka / výška -> exploze */
        if (z >= 6 || r->fuse==0){
            explode_rocket(r);
            r->alive = 0;
        } else {
            r->fuse--;
        }
    }

    /* --- Update jisker --- */
    for(int i=0;i<(int)(sizeof(fw.sparks)/sizeof(fw.sparks[0])); ++i){
        spark_t *s = &fw.sparks[i];
        if(!s->alive) continue;

        /* pohyb + gravitace */
        s->x_fp += s->vx;
        s->y_fp += s->vy;
        s->z_fp += s->vz;
        s->vz   -= 18;  /* gravitace */

        /* volitelný air-drag (zpomalení, ať dřív vyhasnou ve vzduchu) */
        s->vx = (s->vx * FW_AIR_DRAG_NUM) / FW_AIR_DRAG_DEN;
        s->vy = (s->vy * FW_AIR_DRAG_NUM) / FW_AIR_DRAG_DEN;
        s->vz = (s->vz * FW_AIR_DRAG_NUM) / FW_AIR_DRAG_DEN;

        /* tlumení jasu (fade) */
        if((s->life & 1)==0){
            s->r = (uint8_t)((s->r * 220) / 255);
            s->g = (uint8_t)((s->g * 220) / 255);
            s->b = (uint8_t)((s->b * 220) / 255);
        }

        int x = s->x_fp>>8, y = s->y_fp>>8, z = s->z_fp>>8;

        /* ZABÍT DŘÍV VE VZDUCHU: jakmile klesne na z<=FW_GROUND_KILL_Z, hned zmizí */
        if (z <= FW_GROUND_KILL_Z) {
            s->alive = 0;
            continue;
        }

        /* ukončení mimo scénu / dohasnutí */
        if((unsigned)x>7 || (unsigned)y>7 || (unsigned)z>7 || s->life==0 || (s->r|s->g|s->b)==0){
            s->alive = 0;
            continue;
        }

        /* vykreslení jiskry + slabý „ocásek“ o krok zpět */
        add_voxel(x, y, z, s->r, s->g, s->b);
        if (z<7) add_voxel(x, y, z+1, s->r>>3, s->g>>3, s->b>>3);

        s->life--;
    }

}

#include "anim_bees.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

extern Voxel_t scrBuf[8][8][8];

/* ===== Ladění ===== */
#define FRAME_MS      70
#define N_BEES        5
#define N_FLOWERS     3

#define SPEED_MIN     0.10f
#define SPEED_MAX     0.18f
#define STEER_GAIN    0.18f
#define NOISE_GAIN    0.05f
#define ARRIVE_DIST   0.35f
#define DWELL_MIN     10
#define DWELL_MAX     22

/* ===== Mapování os =====
   scrBuf[x][y][z], Y je převrácená -> zapisuj do scrBuf[x][7 - y][z].
*/
static inline int inb(int v){ return (unsigned)v < 8; }
static inline void pset(int x,int y,int z, Voxel_t c){
    if(inb(x)&&inb(y)&&inb(z)) scrBuf[x][7 - y][z] = c;
}

/* Barvy – sytější */
static inline Voxel_t hive_col(void){       return (Voxel_t){120, 70, 20}; }   // tmavě hnědá
static inline Voxel_t bee_col(void){        return (Voxel_t){255, 220,  0}; }  // jasná žluť
static inline Voxel_t bee_shadow_col(void){ return (Voxel_t){120, 100, 20}; }
static inline Voxel_t grass_col(void){      return (Voxel_t){ 15,  60, 15}; }  // jemná tráva
static inline Voxel_t stem_col(void){       return (Voxel_t){ 40, 160, 40}; }  // stonek
static inline Voxel_t flower_col(int i){
    static const Voxel_t pal[] = {
        {255,  60, 140}, { 90, 180, 255}, {255, 210,  60}, {200, 100, 255}
    };
    return pal[i % (int)(sizeof(pal)/sizeof(pal[0]))];
}
static inline Voxel_t dim_col(Voxel_t c){   // hodně ztlumené (≈25 %)
    return (Voxel_t){ (uint8_t)(c.r>>2), (uint8_t)(c.g>>2), (uint8_t)(c.b>>2) };
}

/* Úl: kvádr 2×2×2 v zadním horním rohu (x,y,z ∈ {6,7}) */
static void draw_hive(void){
    Voxel_t c = hive_col();
    for(int z=6; z<=7; ++z)
        for(int y=6; y<=7; ++y)
            for(int x=6; x<=7; ++x)
                pset(x,y,z,c);
}

/* Tráva: vyplň rovinu z=0 */
static void draw_grass(void){
    Voxel_t g = grass_col();
    for(int y=0; y<8; ++y)
        for(int x=0; x<8; ++x)
            pset(x,y,0,g);
}

/* Kytka: křížek (plus) na z=2, ale nejdřív vyplníme 3×3 čtverec ztlumeně.
   Výška h ∈ {1,2} (tedy z=2 a případně i z=3). */
typedef struct { int cx, cy; uint8_t h; Voxel_t col; } flower_t;

static void draw_flower(const flower_t* f){
    const int cx=f->cx, cy=f->cy;
    const Voxel_t c  = f->col;
    const Voxel_t cd = dim_col(c);

    // 1) Ztlumené pozadí 3×3 (z = 2 .. 2+h-1)
    for(int dy=-1; dy<=1; ++dy){
        for(int dx=-1; dx<=1; ++dx){
            const int x=cx+dx, y=cy+dy;
            if(!inb(x)||!inb(y)) continue;
            for(int zz=0; zz<(int)f->h && (2+zz)<=7; ++zz){
                pset(x,y,2+zz, cd);
            }
        }
    }

    // 2) Jasný křížek (střed + N,S,E,W), přepíše ztlumené
    const int dxv[5] = {0, 1,-1, 0, 0};
    const int dyv[5] = {0, 0, 0, 1,-1};
    for(int k=0;k<5;k++){
        const int x=cx+dxv[k], y=cy+dyv[k];
        if(!inb(x)||!inb(y)) continue;
        for(int zz=0; zz<(int)f->h && (2+zz)<=7; ++zz){
            pset(x,y,2+zz, c);
        }
    }

    // 3) Stonek – voxel pod středem na z=1 (propojí s trávou)
    if(inb(cx)&&inb(cy)) pset(cx,cy,1, stem_col());
}

/* Včela: 1 voxel + rychlý stín (minulá integer pozice) */
typedef struct {
    float x,y,z;
    float vx,vy,vz;
    int   targetHive;   // 1=cíl úl, 0=květ
    int   flowerIdx;
    int   dwell;
    int   px,py,pz;     // minulá integer pozice pro stín
} bee_t;

void anim_bees(graph_animation_t *a)
{
    static bee_t B[N_BEES];
    static flower_t F[N_FLOWERS];

    if(a->reset){
        a->reset=0;

        // Tři kytky „naproti úlu“ (předek/levá strana), žádná pod úlem:
        const int pos[N_FLOWERS][2] = { {1,1}, {2,5}, {5,2} };
        for(int i=0;i<N_FLOWERS;i++){
            F[i].cx = pos[i][0];
            F[i].cy = pos[i][1];
            F[i].h  = (uint8_t)(1 + (i % 2));  // 1,2,1...
            F[i].col = flower_col(i);          // syté barvy
        }

        // Včely – start poblíž úlu (zadní horní roh ~6.5,6.5,6.5)
        for(int i=0;i<N_BEES;i++){
            B[i].x = 6.2f + 0.3f*(i&1);
            B[i].y = 6.2f + 0.3f*((i>>1)&1);
            B[i].z = 6.8f - 0.2f*(i%3);
            float ang = (float)(rand()%628)/100.0f;
            float spd = SPEED_MIN + (SPEED_MAX-SPEED_MIN)*((rand()%100)/100.0f);
            B[i].vx = cosf(ang)*spd;
            B[i].vy = sinf(ang)*spd;
            B[i].vz = -spd*0.4f;
            B[i].targetHive = 0;
            B[i].flowerIdx  = i % N_FLOWERS;
            B[i].dwell = 0;
            B[i].px = (int)lroundf(B[i].x);
            B[i].py = (int)lroundf(B[i].y);
            B[i].pz = (int)lroundf(B[i].z);
        }
    }

    a->timer = HAL_GetTick() + FRAME_MS;

    // čistý obraz
    memset(scrBuf, 0, sizeof scrBuf);

    // zem + kytky + úl
    draw_grass();
    for(int i=0;i<N_FLOWERS;i++) draw_flower(&F[i]);
    draw_hive();

    // simulace a kreslení včel
    for(int i=0;i<N_BEES;i++){
        bee_t *b = &B[i];

        // cíle: úl (střed kvádru) vs. květ (lehce nad kytkou)
        float tx,ty,tz;
        if(b->targetHive){
            tx=6.5f; ty=6.5f; tz=6.5f;
        }else{
            tx=(float)F[b->flowerIdx].cx;
            ty=(float)F[b->flowerIdx].cy;
            tz= 2.3f; // lehce nad plátkem kytek
        }

        if(b->dwell>0){
            b->dwell--;
            if(b->dwell==0){
                if(!b->targetHive){ b->targetHive=1; }
                else { b->targetHive=0; b->flowerIdx = rand()%N_FLOWERS; }
            }
        }else{
            // steering + šum
            float dx=tx-b->x, dy=ty-b->y, dz=tz-b->z;
            float dist = sqrtf(dx*dx+dy*dy+dz*dz) + 1e-6f;
            dx/=dist; dy/=dist; dz/=dist;

            float nx=((rand()%200)-100)/100.0f;
            float ny=((rand()%200)-100)/100.0f;
            float nz=((rand()%200)-100)/100.0f;

            float tgt = (SPEED_MIN+SPEED_MAX)*0.5f;
            b->vx = (1.0f-STEER_GAIN)*b->vx + STEER_GAIN*dx*tgt + NOISE_GAIN*nx;
            b->vy = (1.0f-STEER_GAIN)*b->vy + STEER_GAIN*dy*tgt + NOISE_GAIN*ny;
            b->vz = (1.0f-STEER_GAIN)*b->vz + STEER_GAIN*dz*tgt + NOISE_GAIN*nz;

            // normalizace rychlosti
            float v = sqrtf(b->vx*b->vx + b->vy*b->vy + b->vz*b->vz) + 1e-6f;
            float want = SPEED_MIN + (SPEED_MAX-SPEED_MIN)*0.6f;
            float s = want / v; b->vx*=s; b->vy*=s; b->vz*=s;

            // pohyb
            b->x += b->vx; b->y += b->vy; b->z += b->vz;

            // hranice (měkký odraz)
            if(b->x<0.0f){ b->x=0.0f; b->vx=fabsf(b->vx); }
            if(b->x>7.0f){ b->x=7.0f; b->vx=-fabsf(b->vx); }
            if(b->y<0.0f){ b->y=0.0f; b->vy=fabsf(b->vy); }
            if(b->y>7.0f){ b->y=7.0f; b->vy=-fabsf(b->vy); }
            if(b->z<0.0f){ b->z=0.0f; b->vz=fabsf(b->vz); }
            if(b->z>7.0f){ b->z=7.0f; b->vz=-fabsf(b->vz); }

            // přílet
            if( (b->x-tx)*(b->x-tx)+(b->y-ty)*(b->y-ty)+(b->z-tz)*(b->z-tz) < ARRIVE_DIST*ARRIVE_DIST )
                b->dwell = DWELL_MIN + (rand()%(DWELL_MAX-DWELL_MIN+1));
        }

        // vykreslení: stín (minulá pozice), pak tělo
        int xi=(int)lroundf(b->x), yi=(int)lroundf(b->y), zi=(int)lroundf(b->z);
        if(inb(b->px)&&inb(b->py)&&inb(b->pz)) pset(b->px,b->py,b->pz, bee_shadow_col());
        if(inb(xi)&&inb(yi)&&inb(zi))          pset(xi,yi,zi, (a->v.r||a->v.g||a->v.b)? a->v : bee_col());
        b->px=xi; b->py=yi; b->pz=zi;
    }
}

#include "anim_volcano.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

extern Voxel_t scrBuf[8][8][8];

/* ===== ladění ===== */
#define FRAME_MS       60
#define CRATER_Z       5.5f
#define CONE_RADIUS    3.4f
#define CRATER_RADIUS  1.1f
#define ROCK_DARK      1

#define N_LAVA         32
#define N_SMOKE        18
#define ERUPT_PERIOD   25
#define LAVA_GRAVITY   0.045f
#define LAVA_DRAG      0.98f
#define SMOKE_RISE     0.05f
#define SMOKE_DRIFT    0.02f
#define GLOW_DECAY     4
/* ================== */

static inline int inb(int v){ return (unsigned)v < 8; }
static inline void pset(int x,int y,int z, Voxel_t c){
    if(inb(x)&&inb(y)&&inb(z)) scrBuf[x][7 - y][z] = c; // Y flip
}
static inline Voxel_t C(uint8_t r,uint8_t g,uint8_t b){ return (Voxel_t){r,g,b}; }

/* kámen */
static inline Voxel_t rock_col(int z){
    uint8_t base = ROCK_DARK ? 28 : 40;
    uint8_t v = (uint8_t)(base + z*8);
    return (Voxel_t){v, (uint8_t)(v*0.7f), (uint8_t)(v*0.5f)};
}

/* žhavá láva na zemi – jen červená → oranžová */
static inline Voxel_t lava_col_ground(uint8_t h){
    uint8_t r, g;
    if(h < 64){            // tmavě červená
        r = h<<2; g = 0;
    } else if(h < 128){    // plná červená
        r = 255; g = (h-64)<<1; if(g>120) g=120;
    } else {               // červeno-oranžová
        r = 255; g = (uint8_t)(120 + ((h-128)<<1));
        if(g>180) g=180;
    }
    return (Voxel_t){r,g,0};
}

/* letící kapky – jen červená a oranžová */
static inline Voxel_t lava_col_air(uint8_t h){
    uint8_t r = 255;
    uint8_t g = (uint8_t)(80 + (h>>1)); // 80..207
    if(g > 180) g = 180;
    return (Voxel_t){r,g,0};
}

static int H[8][8];
static uint8_t glow[8][8][8];

typedef struct { float x,y,z, vx,vy,vz; uint8_t a; uint8_t on; } particle_t;
static particle_t lava[N_LAVA];
static particle_t smoke[N_SMOKE];

static inline float frand(float a,float b){ return a + (b-a)*(rand()/(float)RAND_MAX); }

static void build_heightmap(void){
    const float cx=3.5f, cy=3.5f;
    for(int y=0;y<8;y++){
        for(int x=0;x<8;x++){
            float dx=x-cx, dy=y-cy;
            float r = sqrtf(dx*dx+dy*dy);
            float zf = CRATER_Z - (r * (CRATER_Z / CONE_RADIUS));
            if(r < CRATER_RADIUS) zf -= 1.5f;
            if(zf < 0.f) zf = 0.f; if(zf > 7.f) zf = 7.f;
            H[x][y] = (int)floorf(zf + 0.5f);
        }
    }
}

static void reset_particles(void){
    for(int i=0;i<N_LAVA;i++)  lava[i].on=0;
    for(int i=0;i<N_SMOKE;i++) smoke[i].on=0;
}

static void emit_from_rim(float *sx,float *sy,float *sz){
    const float cx=3.5f, cy=3.5f;
    float ang = frand(0.f, 6.28318f);
    float r   = CRATER_RADIUS + frand(0.0f, 0.5f);
    float xf = cx + r*cosf(ang);
    float yf = cy + r*sinf(ang);
    int xi=(int)lroundf(xf), yi=(int)lroundf(yf);
    if(!inb(xi)) xi = (xi<0)?0:7; if(!inb(yi)) yi=(yi<0)?0:7;
    *sx = xf;
    *sy = yf;
    *sz = (float)H[xi][yi] + 0.2f;
}

static void erupt(void){
    for(int k=0;k<12;k++){
        int i=-1; for(int j=0;j<N_LAVA;j++){ if(!lava[j].on){ i=j; break; } }
        if(i<0) break;
        lava[i].on=1;
        emit_from_rim(&lava[i].x, &lava[i].y, &lava[i].z);
        float ang = frand(0.f, 6.28318f);
        float spH = frand(0.10f, 0.22f);
        lava[i].vx = cosf(ang)*spH;
        lava[i].vy = sinf(ang)*spH;
        lava[i].vz = frand(0.26f, 0.38f);
        lava[i].a  = 255;
    }
    for(int k=0;k<3;k++){
        int i=-1; for(int j=0;j<N_SMOKE;j++){ if(!smoke[j].on){ i=j; break; } }
        if(i<0) break;
        smoke[i].on=1;
        float sx,sy,sz; emit_from_rim(&sx,&sy,&sz);
        smoke[i].x=sx; smoke[i].y=sy; smoke[i].z=sz+0.2f;
        smoke[i].vx = frand(-SMOKE_DRIFT, SMOKE_DRIFT);
        smoke[i].vy = frand(-SMOKE_DRIFT, SMOKE_DRIFT);
        smoke[i].vz = SMOKE_RISE + frand(0.f, 0.02f);
        smoke[i].a  = (uint8_t)frand(150, 210);
    }
}

static void splash_glow(int x,int y,int z){
    if(!inb(x)||!inb(y)||!inb(z)) return;
    uint16_t v = glow[x][y][z] + 255; glow[x][y][z] = (uint8_t)(v>255?255:v);
    const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
    for(int k=0;k<4;k++){
        int nx=x+dx[k], ny=y+dy[k];
        if(!inb(nx)||!inb(ny)) continue;
        int hz = H[nx][ny];
        if(hz <= z){
            uint16_t vv = glow[nx][ny][hz] + 180;
            glow[nx][ny][hz] = (uint8_t)(vv>255?255:vv);
        }
    }
}

static void step_particles(void){
    for(int i=0;i<N_LAVA;i++){
        if(!lava[i].on) continue;
        lava[i].x += lava[i].vx;
        lava[i].y += lava[i].vy;
        lava[i].z += lava[i].vz;
        lava[i].vx *= LAVA_DRAG;
        lava[i].vy *= LAVA_DRAG;
        lava[i].vz = lava[i].vz*LAVA_DRAG - LAVA_GRAVITY;
        if(lava[i].x<0.f||lava[i].x>7.f||lava[i].y<0.f||lava[i].y>7.f||lava[i].z<0.f||lava[i].z>7.5f){
            lava[i].on=0; continue;
        }
        int xi=(int)lroundf(lava[i].x), yi=(int)lroundf(lava[i].y);
        int zhit = H[xi][yi];
        if(lava[i].z <= (float)zhit + 0.15f){
            splash_glow(xi,yi,zhit);
            lava[i].on=0;
        }
    }
    for(int i=0;i<N_SMOKE;i++){
        if(!smoke[i].on) continue;
        smoke[i].x += smoke[i].vx;
        smoke[i].y += smoke[i].vy;
        smoke[i].z += smoke[i].vz;
        smoke[i].vx += frand(-0.005f,0.005f);
        smoke[i].vy += frand(-0.005f,0.005f);
        if(smoke[i].a>3) smoke[i].a -= 3; else smoke[i].a=0;
        if(smoke[i].z>7.5f || smoke[i].a==0) smoke[i].on=0;
    }
    if((rand()%6)==0){
        int i=-1; for(int j=0;j<N_LAVA;j++){ if(!lava[j].on){ i=j; break; } }
        if(i>=0){
            lava[i].on=1;
            emit_from_rim(&lava[i].x, &lava[i].y, &lava[i].z);
            float ang = frand(0.f, 6.28318f);
            float spH = frand(0.06f, 0.12f);
            lava[i].vx = cosf(ang)*spH;
            lava[i].vy = sinf(ang)*spH;
            lava[i].vz = frand(0.18f, 0.28f);
            lava[i].a  = 220;
        }
    }
}

static void render_scene(void){
    memset(scrBuf, 0, sizeof scrBuf);
    for(int y=0;y<8;y++){
        for(int x=0;x<8;x++){
            int h = H[x][y];
            for(int z=0; z<=h && z<8; z++){
                pset(x,y,z, rock_col(z));
            }
        }
    }
    for(int z=0; z<8; z++){
        for(int y=0; y<8; y++){
            for(int x=0; x<8; x++){
                uint8_t g = glow[x][y][z];
                if(g){
                    pset(x,y,z, lava_col_ground(g));
                    glow[x][y][z] = (g > GLOW_DECAY)? (uint8_t)(g - GLOW_DECAY) : 0;
                }
            }
        }
    }
    for(int i=0;i<N_LAVA;i++){
        if(!lava[i].on) continue;
        int xi=(int)lroundf(lava[i].x), yi=(int)lroundf(lava[i].y), zi=(int)lroundf(lava[i].z);
        if(inb(xi)&&inb(yi)&&inb(zi)){
            pset(xi,yi,zi, lava_col_air(lava[i].a));
        }
    }
    for(int i=0;i<N_SMOKE;i++){
        if(!smoke[i].on) continue;
        int xi=(int)lroundf(smoke[i].x), yi=(int)lroundf(smoke[i].y), zi=(int)lroundf(smoke[i].z);
        if(inb(xi)&&inb(yi)&&inb(zi)){
            uint8_t v = (uint8_t)(smoke[i].a/3); if(v<15) v=15;
            pset(xi,yi,zi, C(v,v,v));
        }
    }
}

void anim_volcano(graph_animation_t *a)
{
    static int seeded = 0;
    static int frame = 0;

    if(a->reset || !seeded){
        build_heightmap();
        memset(glow, 0, sizeof glow);
        reset_particles();
        frame = 0;
        seeded = 1;
        a->reset = 0;
    }

    a->timer = HAL_GetTick() + FRAME_MS;
    frame++;

    if(frame % ERUPT_PERIOD == 0){
        erupt();
    }

    step_particles();
    render_scene();
}

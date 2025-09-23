#include "anim_heartbeat.h"
#include <stdint.h>
#include <string.h>

/* extern v projektu (pokud nejsou v hlavičce) */
// extern Voxel_t scrBuf[8][8][8];
// uint32_t HAL_GetTick(void);

/* -----------------------------------------------------------
   Heartbeat Sphere (lub-dub): dva rychlé pulzy a pauza
   - bez floatů; koule v centru (3.5,3.5,3.5)
   - radius v "half-voxel" jednotkách (0.5 vox)
   - vyplněná koule + slabší světlejší skořápka
   ----------------------------------------------------------- */

typedef struct {
    uint8_t phase;      /* 0..255 – běh cyklu */
} hb_state_t;

static hb_state_t st;

/* uživatelské laditelné konstanty */
#define HB_TIMER_MS        70      /* tempo snímků (nižší = rychlejší) */
#define HB_BASE_R_H        4       /* základní poloměr v half-vox – 4 => 2.0 vox */
#define HB_BOOST_R_H       3       /* přídavek poloměru při max pulzu (v half-vox) */
#define HB_SHELL_BAND_H2   10      /* tloušťka skořápky ve "half-vox^2" (~lesk na povrchu) */

/* dvojúder: centra pulzů (v "phase" 0..255) a šířky (trojúhelník) */
#define HB_P1_CENTER       28
#define HB_P1_WIDTH        22      /* šířka 1. pulzu */
#define HB_P1_AMP          255     /* amplituda 1. pulzu (0..255) */

#define HB_P2_CENTER       68
#define HB_P2_WIDTH        18      /* šířka 2. pulzu (kratší, ostřejší) */
#define HB_P2_AMP          230     /* amplituda 2. pulzu (trochu nižší) */

/* zbytek cyklu klid (pauza); můžeš posunout rychlost změnou HB_TIMER_MS */
#define HB_PHASE_INC       6       /* posun fáze za snímek (větší = kratší cyklus) */

/* barvy (základ + přídavek s pulzem) */
#define HB_BASE_R_COL      90
#define HB_BASE_G_COL      0
#define HB_BASE_B_COL      0
#define HB_BOOST_R_COL     165     /* o kolik zesílí R při max pulzu */
#define HB_BOOST_G_COL     40      /* o kolik se přimíchá G (bílo-červený nádech) */
#define HB_BOOST_B_COL     20      /* lehká teplá bílá */

/* -------- pomocné funkce -------- */

/* minimální kruhová vzdálenost v 0..255 prostoru */
static inline uint8_t circ_dist8(uint8_t a, uint8_t b){
    uint8_t d = (a > b) ? (a - b) : (b - a);
    return (d <= 128) ? d : (uint8_t)(256 - d);
}

/* trojúhelníkový pulz: max v C, klesá k nule šířkou W; vrací 0..255 */
static inline uint8_t tri_pulse(uint8_t phase, uint8_t C, uint8_t W, uint8_t amp){
    uint8_t d = circ_dist8(phase, C);
    if (d >= W) return 0;
    /* lineárně z W..0 -> 0..amp */
    uint16_t v = (uint16_t)(W - d) * amp;
    return (uint8_t)(v / W);
}

/* saturace na 0..255 */
static inline uint8_t clamp8i(int v){
    return (v < 0) ? 0 : (v > 255) ? 255 : (uint8_t)v;
}

/* ----------------------------------------------------------- */

void anim_heartbeat(graph_animation_t *a)
{
    if (a->reset){
        a->reset = 0;
        st.phase = 0;
    }

    a->timer = HAL_GetTick() + HB_TIMER_MS;

    /* čisté plátno (u heartbeat chceme ostré snímky bez globálního afterglow) */
    memset(scrBuf, 0, sizeof(scrBuf));

    /* obálka: dva pulzy v rámci jednoho cyklu */
    uint16_t e1 = tri_pulse(st.phase, HB_P1_CENTER, HB_P1_WIDTH, HB_P1_AMP);
    uint16_t e2 = tri_pulse(st.phase, HB_P2_CENTER, HB_P2_WIDTH, HB_P2_AMP);
    uint16_t env = e1 + e2;                 /* 0..(255+230) */
    if (env > 255) env = 255;               /* saturace na 0..255 */

    /* poloměr v half-vox jednotkách */
    int R_h = HB_BASE_R_H + (int)((env * HB_BOOST_R_H) / 255); /* 4..(4+3)=7 */

    /* jas barvy – základ + závislý na pulzu */
    int Rcol = HB_BASE_R_COL + (int)((env * HB_BOOST_R_COL) / 255);
    int Gcol = HB_BASE_G_COL + (int)((env * HB_BOOST_G_COL) / 255);
    int Bcol = HB_BASE_B_COL + (int)((env * HB_BOOST_B_COL) / 255);

    uint8_t baseR = clamp8i(Rcol);
    uint8_t baseG = clamp8i(Gcol);
    uint8_t baseB = clamp8i(Bcol);

    /* sférické vykreslení: používáme "half-vox" souřadnice
       - střed (3.5,3.5,3.5) -> v polo-voxel jednotkách je to (7,7,7)
       - voxel (x,y,z) ~ bod (2x,2y,2z) v half-vox
       - patří do koule, pokud (2x-7)^2 + (2y-7)^2 + (2z-7)^2 <= R_h^2
    */
    int Rh2 = R_h * R_h;
    for (int x=0; x<8; ++x){
        int dxh = (x<<1) - 7;     /* 2x - 7  (half-vox) */
        int dxh2 = dxh*dxh;
        for (int y=0; y<8; ++y){
            int dyh = (y<<1) - 7;
            int dyh2 = dyh*dyh;
            for (int z=0; z<8; ++z){
                int dzh = (z<<1) - 7;
                int dzh2 = dzh*dzh;

                int d2 = dxh2 + dyh2 + dzh2;

                if (d2 <= Rh2){
                    /* uvnitř koule */
                    scrBuf[x][y][z].r = baseR;
                    scrBuf[x][y][z].g = baseG;
                    scrBuf[x][y][z].b = baseB;

                    /* zvýrazni skořápku: pokud jsme v pásmu blízko povrchu,
                       přidej jemné zesvětlení (lesk). */
                    if (Rh2 - d2 <= HB_SHELL_BAND_H2){
                        int add = 40; /* lesk – můžeš změkčit/zesílit */
                        int r = scrBuf[x][y][z].r + add;
                        int g = scrBuf[x][y][z].g + (add>>1);
                        int b = scrBuf[x][y][z].b + (add>>1);
                        scrBuf[x][y][z].r = (uint8_t)(r>255?255:r);
                        scrBuf[x][y][z].g = (uint8_t)(g>255?255:g);
                        scrBuf[x][y][z].b = (uint8_t)(b>255?255:b);
                    }
                }
            }
        }
    }

    /* posuň fázi – dva pulzy proběhnou na začátku cyklu, zbytek pauza */
    st.phase += HB_PHASE_INC;
}

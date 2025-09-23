#include "anim_text.h"
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

extern Voxel_t scrBuf[8][8][8];

/* ====== LADĚNÍ ORIENTACE A RYCHLOSTI ====== */
#define FLIP_X    1   // 1 = zrcadlit X (sloupce glyphu)
#define FLIP_Y    1   // 1 = zrcadlit Y (směr pohybu textu)
#define FLIP_Z    1   // 1 = zrcadlit Z (řádky glyphu)
#define FRAME_MS  50  // perioda snímku v ms
#define STEP_DIV  2   // posun o 1 voxel po každých STEP_DIV framech
/* ========================================== */

/* --- 5×7 font: bit4 je levý sloupec, bit0 pravý (šířka 5) --- */
typedef struct { char ch; uint8_t row[7]; } glyph_t;
/* Řádky jsou shora dolů (0..6). Každý řádek: b4 b3 b2 b1 b0 (← levý … pravý →) */
static const glyph_t FONT[] = {
    // A–Z
    {'A',{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'D',{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
    {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G',{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}},
    {'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I',{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
    {'J',{0x01,0x01,0x01,0x01,0x11,0x11,0x0E}},
    {'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M',{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
    {'N',{0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q',{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
    {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W',{0x11,0x11,0x11,0x15,0x15,0x1B,0x11}},
    {'X',{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
    {'Y',{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
    // 0–9
    {'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3',{0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}},
    {'4',{0x11,0x11,0x11,0x1F,0x01,0x01,0x01}},
    {'5',{0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}},
    {'6',{0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}},
    {'7',{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
    {'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9',{0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}},
    // mezera
    {' ',{0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
};
static const glyph_t* find_glyph(char c){
    c = (char)toupper((unsigned char)c);
    for (size_t i=0;i<sizeof(FONT)/sizeof(FONT[0]);++i)
        if (FONT[i].ch==c) return &FONT[i];
    // fallback na mezeru
    for (size_t i=0;i<sizeof(FONT)/sizeof(FONT[0]);++i)
        if (FONT[i].ch==' ') return &FONT[i];
    return NULL;
}

/* Kreslení jednoho glyphu do roviny Y=y (vycentrované v X=1..5, Z=0..6). */
static inline void draw_glyph_at_y(const glyph_t* g, int y, Voxel_t col){
    if (!g) return;
    if (y<0 || y>7) return;
    const int yy = FLIP_Y ? (7 - y) : y;
    const int x0 = 1;  // 5 sloupců → posuneme na střed 1..5

    for (int zr=0; zr<7; ++zr){
        int z = FLIP_Z ? (6 - zr) : zr;
        if ((unsigned)z >= 8) continue;
        uint8_t row = g->row[zr]; // bit4..bit0 (←→)
        for (int xc=0; xc<5; ++xc){
            int bitIndex = FLIP_X ? xc : (4 - xc); // bez flipu: xc=0→bit4 (levý)
            if ((row >> bitIndex) & 1u){
                int x = x0 + xc;
                if ((unsigned)x<8) scrBuf[x][yy][z] = col;
            }
        }
    }
}

void anim_text(graph_animation_t *a)
{
    // Tvůj text (kapitálky). Klidně přepoj na a->text, pokud ho máš.
    static const char *msg = "HAPPY BIRTHDAY 50";
    // Jak dlouhá má být mezera při ' ' i na konci
    const int gap_frames = 8;

    static int iChar;        // index znaku v msg
    static int y;            // pozice roviny (0..7)
    static int gap;          // odpočítávání pauzy
    static int frameDiv;     // dělič posunu (zpomalení)
    static Voxel_t col;      // barva písmen

    if (a->reset){
        a->reset = 0;
        iChar = 0;
        y = 0;
        gap = 0;
        frameDiv = 0;
        col = (a->v.r||a->v.g||a->v.b) ? a->v : (Voxel_t){255,255,255};
    }

    a->timer = HAL_GetTick() + FRAME_MS;
    memset(scrBuf, 0, sizeof scrBuf);

    // pokud právě běží pauza, odtikni
    if (gap > 0){
        if (--gap == 0){
            // po pauze posuň na další znak (nebo restartuj text)
            iChar++;
            if (msg[iChar] == '\0'){ // konec textu → restart
                iChar = 0;
                y = 0;
                if (!(a->v.r||a->v.g||a->v.b)){
                    col = (Voxel_t){
                        (uint8_t)(200 + rand()%56),
                        (uint8_t)(200 + rand()%56),
                        (uint8_t)(200 + rand()%56)
                    };
                }
            }
        }
        return;
    }

    char c = msg[iChar];

    // pokud je mezera, rovnou vlož rychlý „gap“ a nic nekresli
    if (c == ' '){
        gap = gap_frames;
        return;
    }

    // vykresli aktuální znak (v rovině y) + 1-voxel stín (y-1)
    const glyph_t* g = find_glyph(c);
    draw_glyph_at_y(g, y, col);
    if (y-1 >= 0){
        Voxel_t shade = (Voxel_t){ (uint8_t)(col.r>>1), (uint8_t)(col.g>>1), (uint8_t)(col.b>>1) };
        draw_glyph_at_y(g, y-1, shade);
    }

    // zpomalený posun – posuň až každý STEP_DIV-tý frame
    frameDiv = (frameDiv + 1) % STEP_DIV;
    if (frameDiv == 0){
        y++;
        if (y > 7){
            // znak „dojel“, přejdi na další
            y = 0;
            iChar++;
            if (msg[iChar] == '\0'){
                // konec textu -> prázdná pauza, pak od začátku
                gap = gap_frames;
            }
        }
    }
}

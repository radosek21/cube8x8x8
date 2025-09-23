#include "anim_scanner.h"
#include <stdlib.h>
#include <string.h>

static inline Voxel_t rand_color(void){
    // sytější, ale ne úplně max
    uint8_t r = 64 + (rand() % 192);
    uint8_t g = 64 + (rand() % 192);
    uint8_t b = 64 + (rand() % 192);
    return (Voxel_t){r,g,b};
}

// ztmavení barvy bitovým posunem (rychlé)
static inline Voxel_t shade(Voxel_t c, int shift){
    // shift=1 -> ~50 %, shift=2 -> ~25 %
    return (Voxel_t){ (uint8_t)(c.r >> shift),
                      (uint8_t)(c.g >> shift),
                      (uint8_t)(c.b >> shift) };
}

void anim_scanner(graph_animation_t *a)
{
    static int dir = +1;          // +1 vpřed, -1 zpět
    static Voxel_t col;           // aktuální barva

    if(a->reset){
        a->reset = 0;
        if(a->seqIndex > 2) a->seqIndex = 0;
        if(a->i1 < 0 || a->i1 > 7) a->i1 = 0;
        dir = +1;
        col = rand_color();
    }

    a->timer = HAL_GetTick() + 100;

    memset(scrBuf, 0, sizeof scrBuf);

    // hlavní index roviny a trailing stíny (za rovinou, ve směru pohybu)
    int m  = a->i1;
    int s1 = m - dir;     // 1 voxel za rovinou
    int s2 = m - 2*dir;   // 2 voxely za rovinou

    Voxel_t c0 = col;             // hlavní barva
    Voxel_t c1 = shade(col, 1);   // ~50 %
    Voxel_t c2 = shade(col, 2);   // ~25 %

    switch (a->seqIndex)
    {
        case 0: // skenujeme v ose X (rovina YZ na x=m)
            if(m>=0 && m<8)
                for (int y = 0; y < 8; ++y)
                    for (int z = 0; z < 8; ++z)
                        scrBuf[m][y][z] = c0;

            if(s1>=0 && s1<8)
                for (int y = 0; y < 8; ++y)
                    for (int z = 0; z < 8; ++z)
                        scrBuf[s1][y][z] = c1;

            if(s2>=0 && s2<8)
                for (int y = 0; y < 8; ++y)
                    for (int z = 0; z < 8; ++z)
                        scrBuf[s2][y][z] = c2;
            break;

        case 1: // osa Y (rovina XZ na y=m)
            if(m>=0 && m<8)
                for (int x = 0; x < 8; ++x)
                    for (int z = 0; z < 8; ++z)
                        scrBuf[x][m][z] = c0;

            if(s1>=0 && s1<8)
                for (int x = 0; x < 8; ++x)
                    for (int z = 0; z < 8; ++z)
                        scrBuf[x][s1][z] = c1;

            if(s2>=0 && s2<8)
                for (int x = 0; x < 8; ++x)
                    for (int z = 0; z < 8; ++z)
                        scrBuf[x][s2][z] = c2;
            break;

        case 2: // osa Z (rovina XY na z=m)
            if(m>=0 && m<8)
                for (int x = 0; x < 8; ++x)
                    for (int y = 0; y < 8; ++y)
                        scrBuf[x][y][m] = c0;

            if(s1>=0 && s1<8)
                for (int x = 0; x < 8; ++x)
                    for (int y = 0; y < 8; ++y)
                        scrBuf[x][y][s1] = c1;

            if(s2>=0 && s2<8)
                for (int x = 0; x < 8; ++x)
                    for (int y = 0; y < 8; ++y)
                        scrBuf[x][y][s2] = c2;
            break;

        default:
            a->seqIndex = 0;
            dir = +1;
            col = rand_color();
            break;
    }

    // pohyb + ping-pong s přepínáním osy a novou barvou při odrazu
    a->i1 += dir;

    // odraz na hraně aktuální osy
    if (a->i1 >= 7) {
        a->i1 = 7;
        dir = -1;
        col = rand_color();                 // nová barva po odrazu
    } else if (a->i1 <= 0) {
        a->i1 = 0;
        dir = +1;
        // při návratu na začátek přepneme osu a změníme barvu
        a->seqIndex = (a->seqIndex + 1) % 3;
        col = rand_color();
    }
}

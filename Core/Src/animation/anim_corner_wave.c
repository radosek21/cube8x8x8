/*
 * anim_edges.c
 *
 *  Created on: Sep 19, 2025
 *      Author: libor
 */

#include "anim_corner_wave.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

static inline uint8_t lerp(uint8_t a, uint8_t b, float t)
{
    return (uint8_t)(a + t * (b - a));
}

void anim_corner_wave(graph_animation_t *a)
{
    if (!a) return;

    a->timer = HAL_GetTick() + 75; // krok 50ms
    const int max_dist = 21;       // 7+7+7

    // fade-out starých voxelů (0.9 = 10% stmívání)
    for (int x = 0; x < 8; x++)
        for (int y = 0; y < 8; y++)
            for (int z = 0; z < 8; z++)
            {
                scrBuf[x][y][z].r = (scrBuf[x][y][z].r * 9) / 10;
                scrBuf[x][y][z].g = (scrBuf[x][y][z].g * 9) / 10;
                scrBuf[x][y][z].b = (scrBuf[x][y][z].b * 9) / 10;
            }

    // určujeme start a end barvu podle aktuálního přejezdu
    uint8_t r_start, g_start, b_start;
    uint8_t r_end, g_end, b_end;

    switch (a->seqIndex % 3)
    {
        case 0: // modrá → zelená
            r_start = 0; g_start = 0; b_start = 255;
            r_end   = 0; g_end   = 255; b_end   = 0;
            break;
        case 1: // zelená → červená
            r_start = 0; g_start = 255; b_start = 0;
            r_end   = 255; g_end   = 0; b_end   = 0;
            break;
        case 2: // červená → modrá
            r_start = 255; g_start = 0; b_start = 0;
            r_end   = 0; g_end   = 0; b_end   = 255;
            break;
    }

    // iterace přes všechny voxely
    for (int x = 0; x < 8; x++)
        for (int y = 0; y < 8; y++)
            for (int z = 0; z < 8; z++)
            {
                int dist = x + y + z;
                int dist_to_front = a->i1 - dist;

                if (dist_to_front >= -2 && dist_to_front <= 2) // fronta ±2 voxely
                {
                    float t_front = 1.0f - (fabsf(dist_to_front) / 3.0f); // lineární pokles
                    t_front = fmax(0.0f, fmin(1.0f, t_front));           // omezíme 0..1

                    float t_color = (float)dist / max_dist;

                    a->v.r = lerp(r_start, r_end, t_color);
                    a->v.g = lerp(g_start, g_end, t_color);
                    a->v.b = lerp(b_start, b_end, t_color);

                    // násobení faktorem fronty pro zvýraznění středu
                    scrBuf[x][y][z].r = fmin(scrBuf[x][y][z].r + (uint8_t)(a->v.r * t_front), 255);
                    scrBuf[x][y][z].g = fmin(scrBuf[x][y][z].g + (uint8_t)(a->v.g * t_front), 255);
                    scrBuf[x][y][z].b = fmin(scrBuf[x][y][z].b + (uint8_t)(a->v.b * t_front), 255);
                }
            }

    // posun fronty
    a->i1++;
    if (a->i1 > max_dist)
    {
        a->i1 = 0;
        a->seqIndex = (a->seqIndex + 1) % 3; // další barevný přejezd
    }
}


/*
 * anim_scanner.c
 *
 *  Created on: Sep 19, 2025
 *      Author: libor
 */

#include "anim_scanner.h"


void anim_scanner(graph_animation_t *a)
{
    a->timer = HAL_GetTick() + 100;

    memset(scrBuf, 0, sizeof scrBuf);

    switch (a->seqIndex)
    {
        case 0:
            VOXEL_COL_MAGENTA(a->v);
            for (int y = 0; y < 8; ++y)
                for (int z = 0; z < 8; ++z)
                    scrBuf[a->i1][y][z] = a->v;
            break;

        case 1:
            VOXEL_COL_CYAN(a->v);
            for (int x = 0; x < 8; ++x)
                for (int z = 0; z < 8; ++z)
                    scrBuf[x][a->i1][z] = a->v;
            break;

        case 2:
            VOXEL_COL_YELLOW(a->v);
            for (int x = 0; x < 8; ++x)
                for (int y = 0; y < 8; ++y)
                    scrBuf[x][y][a->i1] = a->v;
            break;

        default:
            a->seqIndex = 0;
            break;
    }

    /* inkrementace s ošetřením rozsahu */
    a->i1++;
    if (a->i1 >= 8)
    {
        a->i1 = 0;
        a->seqIndex = (a->seqIndex + 1) % 3; // cyklus 0..2
    }
}

/*
 * anim_edges.c
 *
 *  Created on: Sep 19, 2025
 *      Author: libor
 */

#include "anim_edges.h"



void anim_edges(graph_animation_t *a)
{
  a->timer = HAL_GetTick() + 100;
  memset((uint8_t *)scrBuf, 0, sizeof(Voxel_t)*8*8*8);

  switch (a->seqIndex)
  {
    case 0: case 1: VOXEL_COL_RED(a->v);    break;
    case 2: case 3: VOXEL_COL_GREEN(a->v);  break;
    case 4: case 5: VOXEL_COL_BLUE(a->v);   break;
  }

  switch (a->seqIndex)
  {
    case 0: case 2: case 4:
      for (int i = 0; i < a->i1; i++)
      {
        scrBuf[i][0][0] = a->v; scrBuf[0][i][0] = a->v; scrBuf[0][0][i] = a->v;
      }

      a->i1++;
      if (a->i1 < 8) { break; }
      a->i1 = 8;

      for (int i = 0; i < a->i2; i++)
      {
        scrBuf[7][i][0] = a->v; scrBuf[7][0][i] = a->v;
        scrBuf[i][7][0] = a->v; scrBuf[0][7][i] = a->v;
        scrBuf[i][0][7] = a->v; scrBuf[0][i][7] = a->v;
      }

      a->i2++;
      if (a->i2 < 8) { break; }
      a->i2 = 8;

      for (int i = 0; i < a->i3; i++)
      {
        scrBuf[i][7][7] = a->v; scrBuf[7][i][7] = a->v; scrBuf[7][7][i] = a->v;
      }

      a->i3++;
      if (a->i3 < 8) { break; }
      a->i3 = 8;


      a->i1 = 0;
      a->i2 = 0;
      a->i3 = 0;
      a->seqIndex++;
      break;

    case 1: case 3: case 5:
      for (int i = a->i1; i < 8; i++)
      {
        scrBuf[i][0][0] = a->v; scrBuf[0][i][0] = a->v; scrBuf[0][0][i] = a->v;
      }

      for (int i = a->i2; i < 8; i++)
      {
        scrBuf[7][i][0] = a->v; scrBuf[7][0][i] = a->v;
        scrBuf[i][7][0] = a->v; scrBuf[0][7][i] = a->v;
        scrBuf[i][0][7] = a->v; scrBuf[0][i][7] = a->v;
      }

      for (int i = a->i3; i < 8; i++)
      {
        scrBuf[i][7][7] = a->v; scrBuf[7][i][7] = a->v; scrBuf[7][7][i] = a->v;
      }


      a->i1++;
      if (a->i1 >= 8)
      {
        a->i1 = 8;
        a->i2++;
        if (a->i2 >= 8)
        {
          a->i2 = 8;
          a->i3++;
          if (a->i3 >= 8)
          {
            a->i1 = 0;
            a->i2 = 0;
            a->i3 = 0;
            a->seqIndex++;
          }
        }
      }
      break;

    default:
      a->seqIndex = 0;
      break;
  }

}



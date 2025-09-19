
/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "graphics.h"
#include "cube_mux.h"
#include "fatfs.h"
#include "string.h"


#define BUFFER_SIZE 4096
#define SEARCH_STR1 "SIZE"
#define SEARCH_STR2 "XYZI"
#define SEARCH_STR3 "RGBA"

typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t z;
  uint8_t i;
} Coord_t;

Voxel_t scrBuf[8][8][8];


#define TICK_EXPIRED(a)           ( HAL_GetTick() - (a) < 0x7fffffff )
#define ANIMATION_FRAME_TIME_MS   ( 100 )

typedef struct
{
  uint32_t timer;
  uint32_t seqIndex;
  Voxel_t v;
  uint32_t i1, i2, i3;

} graph_animation_t;


static graph_animation_t anim;


static void Graphics_AnimEdges();
static void Graphics_AnimSides();



static void Graphics_AnimEdges()
{
  anim.timer = HAL_GetTick() + 50;
  memset(scrBuf, 0, sizeof(Voxel_t)*8*8*8);

  switch (anim.seqIndex)
  {
    case 0:
    case 1:
      anim.v.r = 255;
      anim.v.g = 0;
      anim.v.b = 0;
      break;
    case 2:
    case 3:
      anim.v.r = 0;
      anim.v.g = 255;
      anim.v.b = 0;
      break;
    case 4:
    case 5:
      anim.v.r = 0;
      anim.v.g = 0;
      anim.v.b = 255;
      break;
  }

  switch (anim.seqIndex)
  {
    case 0:
    case 2:
    case 4:
      for (int i = 0; i < anim.i1; i++)
      {
        memcpy(&scrBuf[i][0][0], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[0][i][0], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[0][0][i], (uint8_t *)&anim.v, sizeof(Voxel_t));
      }

      anim.i1++;
      if (anim.i1 < 8) { break; }
      anim.i1 = 8;

      for (int i = 0; i < anim.i2; i++)
      {
        memcpy(&scrBuf[7][i][0], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[7][0][i], (uint8_t *)&anim.v, sizeof(Voxel_t));

        memcpy(&scrBuf[i][7][0], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[0][7][i], (uint8_t *)&anim.v, sizeof(Voxel_t));

        memcpy(&scrBuf[i][0][7], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[0][i][7], (uint8_t *)&anim.v, sizeof(Voxel_t));
      }

      anim.i2++;
      if (anim.i2 < 8) { break; }
      anim.i2 = 8;

      for (int i = 0; i < anim.i3; i++)
      {
        memcpy(&scrBuf[i][7][7], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[7][i][7], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[7][7][i], (uint8_t *)&anim.v, sizeof(Voxel_t));
      }

      anim.i3++;
      if (anim.i3 < 8) { break; }
      anim.i3 = 8;


      anim.i1 = 0;
      anim.i2 = 0;
      anim.i3 = 0;
      anim.seqIndex++;
      break;

    case 1:
    case 3:
    case 5:
      for (int i = anim.i1; i < 8; i++)
      {
        memcpy(&scrBuf[i][0][0], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[0][i][0], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[0][0][i], (uint8_t *)&anim.v, sizeof(Voxel_t));
      }

      for (int i = anim.i2; i < 8; i++)
      {
        memcpy(&scrBuf[7][i][0], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[7][0][i], (uint8_t *)&anim.v, sizeof(Voxel_t));

        memcpy(&scrBuf[i][7][0], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[0][7][i], (uint8_t *)&anim.v, sizeof(Voxel_t));

        memcpy(&scrBuf[i][0][7], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[0][i][7], (uint8_t *)&anim.v, sizeof(Voxel_t));
      }

      for (int i = anim.i3; i < 8; i++)
      {
        memcpy(&scrBuf[i][7][7], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[7][i][7], (uint8_t *)&anim.v, sizeof(Voxel_t));
        memcpy(&scrBuf[7][7][i], (uint8_t *)&anim.v, sizeof(Voxel_t));
      }


      anim.i1++;
      if (anim.i1 >= 8)
      {
        anim.i1 = 8;
        anim.i2++;
        if (anim.i2 >= 8)
        {
          anim.i2 = 8;
          anim.i3++;
          if (anim.i3 >= 8)
          {
            anim.i1 = 0;
            anim.i2 = 0;
            anim.i3 = 0;
            anim.seqIndex++;
          }
        }
      }
      break;

    default:
      anim.seqIndex = 0;
      break;
  }

}

static void Graphics_AnimSides()
{
  anim.timer = HAL_GetTick() + ANIMATION_FRAME_TIME_MS;
  memset(scrBuf, 0, sizeof(Voxel_t)*8*8*8);

  switch(anim.seqIndex)
  {
    case 0:
    {
      anim.v.b = 0;
      anim.v.g = 255;
      anim.v.r = 0;

      for (int i2 = 0; i2 < 8; i2++)
      {
        for (int i3 = 0; i3 < 8; i3++)
        {
          memcpy(&scrBuf[anim.i1][i2][i3], (uint8_t *)&anim.v, sizeof(Voxel_t));
        }
      }

      break;
    }

    case 1:
    {
      anim.v.b = 255;
      anim.v.g = 0;
      anim.v.r = 0;

      for (int i2 = 0; i2 < 8; i2++)
      {
        for (int i3 = 0; i3 < 8; i3++)
        {
          memcpy(&scrBuf[i2][anim.i1][i3], (uint8_t *)&anim.v, sizeof(Voxel_t));
        }
      }
      break;
    }

    case 2:
    {
      anim.v.b = 0;
      anim.v.g = 0;
      anim.v.r = 255;

      for (int i2 = 0; i2 < 8; i2++)
      {
        for (int i3 = 0; i3 < 8; i3++)
        {
          memcpy(&scrBuf[i2][i3][anim.i1], (uint8_t *)&anim.v, sizeof(Voxel_t));
        }
      }
      break;
    }

    default:
    {
      anim.seqIndex = 0;
      break;
    }
  }

  anim.i1++;
  if (anim.i1 >= 8)
  {
    anim.i1 = 0;
    anim.seqIndex++;
    if (anim.seqIndex >= 3)
    {
      anim.seqIndex = 0;
    }
  }

  CubeMux_ScreenToVoxels(scrBuf);
}



void Graphics_ShowAnimation(void)
{
  if (TICK_EXPIRED(anim.timer))
  {
    Graphics_AnimEdges();
    CubeMux_ScreenToVoxels(scrBuf);
  }
}




void Graphics_ShowVoxFile(char *filename)
{
  uint32_t voxelCount;
  Coord_t *coord;
  FIL Fil;
  FRESULT FR_Status;
  uint32_t buffer[BUFFER_SIZE/4];
  size_t bytes_read;

  int sizePos = 0, xyziPos = 0, rgbaPos = 0;

  memset(scrBuf, 0, sizeof(Voxel_t)*8*8*8);

  FR_Status = f_open(&Fil, filename, FA_READ);
  if(FR_Status != FR_OK) {
    return;
  }
  if(f_read(&Fil, buffer, BUFFER_SIZE, &bytes_read) != FR_OK) {
    return;
  }

  f_close(&Fil);

  // Search for "SIZE" string in the buffer
  for (size_t i = 0; i <= (bytes_read - strlen(SEARCH_STR1))/4; i++) {
    if (memcmp(buffer + i, SEARCH_STR1, strlen(SEARCH_STR1)) == 0) {
      sizePos = i;
      break;
    }
  }

  // Search for "XYZI" string in the buffer
  for (size_t i = 0; i <= (bytes_read - strlen(SEARCH_STR2))/4; i++) {
    if (memcmp(buffer + i, SEARCH_STR2, strlen(SEARCH_STR2)) == 0) {
      xyziPos = i;
      break;
    }
  }

  // Search for "RGBA" string in the buffer
  for (size_t i = 0; i <= (bytes_read - strlen(SEARCH_STR3))/4; i++) {
    if (memcmp(buffer + i, SEARCH_STR3, strlen(SEARCH_STR3)) == 0) {
      rgbaPos = i;
      break;
    }
  }

  if ((sizePos > 0) &&
    (rgbaPos > 0) &&
    (xyziPos > 0) &&
    (buffer[sizePos+3] == 8) && (buffer[sizePos+4] == 8) && (buffer[sizePos+5] == 8) &&
    (buffer[rgbaPos+1] == 1024)) {
    voxelCount = buffer[xyziPos + 3];
    coord = (Coord_t*)(buffer + xyziPos + 4);
    rgbaPos += 2;
    for(int i = 0; i < voxelCount; i++) {
      memcpy(&scrBuf[coord->x][coord->y][coord->z], &buffer[rgbaPos + coord->i], sizeof(Voxel_t));
      coord++;
    }
    CubeMux_ScreenToVoxels(scrBuf);
  }

}

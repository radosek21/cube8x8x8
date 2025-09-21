
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

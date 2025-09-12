/*
 * app.c
 *
 *  Created on: Jun 24, 2025
 *      Author: radek.vanhara
 */
#include "app.h"
#include "main.h"
#include "cube_mux.h"
#include "sdcard.h"
#include "encoder.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "fatfs.h"
#include "string.h"



#define BUFFER_SIZE 16
#define SEARCH_STR1 "SIZE"
#define SEARCH_STR2 "XYZI"
#define SEARCH_STR3 "RGBA"

char **filelist;
uint32_t pos = 0;
char *filename;
Voxel_t scrBuf[8][8][8];


void App_LoadVoxToScreen(char *filename);
void App_ScreenToPixels();

typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t z;
  uint8_t i;
} Coord_t;

void App_Init()
{
  CubeMux_Init();
  Sdcard_Init();
  Sdcard_Mount();
  Encoder_Init();
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(4, 6);
  ssd1306_WriteString("Nazdarek", Font_11x18, White);
  ssd1306_UpdateScreen();
  filelist= Sdcard_GetFilesList();
  pos = 0;
//  ptr = filelist[pos];
  CubeMux_StartMux();
}

void App_Handler()
{
  if(Encoder_GetRelativePosition()!=0) {
  pos++;

    filename = filelist[++pos];
    if (filename == NULL) {
      pos = 0;
      filename = filelist[pos];
    }
    ssd1306_Fill(Black);
    ssd1306_SetCursor(1, 10);
    ssd1306_WriteString(filename, Font_7x10, White);
    ssd1306_UpdateScreen();

    App_LoadVoxToScreen(filename);
    App_ScreenToPixels();
  /*if(pos>=8) {
	  pos = 0;
  }
  ssd1306_Fill(Black);
  ssd1306_SetCursor(1, 10);
  char s[32];
  sprintf(s, "Z = %d", pos);
  ssd1306_WriteString(s, Font_7x10, White);
  ssd1306_UpdateScreen();
  for(int z=0; z<8; z++) {
	for(int y=0; y<8; y++) {
	  for(int x=0; x<8; x++) {
		scrBuf[x][y][z].r = (z==pos) ? 255 : 0;
		scrBuf[x][y][z].g = (z==pos) ? 255 : 0;
		scrBuf[x][y][z].b = (z==pos) ? 255 : 0;
	  }
	}
  }
  App_ScreenToPixels();*/
  }
  HAL_Delay(1);
}

void App_LoadVoxToScreen(char *filename)
{
  unsigned long buf[4];
  unsigned long voxelCount;
  unsigned long paletteSize;
  uint8_t palette[1024];
  Coord_t coord;
  FIL Fil;
  FRESULT FR_Status;

  memset(scrBuf, 0, sizeof(Voxel_t)*8*8*8);

  FR_Status = f_open(&Fil, filename, FA_READ);
  if(FR_Status != FR_OK)
  {
    return;
  }

  char buffer[BUFFER_SIZE];
  size_t bytes_read;
  int sizePos = 0, xyziPos = 0, rgbaPos = 0;

  // Čti soubor po částech
  ;
  while ((f_read(&Fil, buffer, BUFFER_SIZE, &bytes_read) == FR_OK) && (bytes_read == BUFFER_SIZE)) {
    // Hledej "SIZE" v bufferu
    for (size_t i = 0; i <= bytes_read - strlen(SEARCH_STR1); i++) {
      if (memcmp(buffer + i, SEARCH_STR1, strlen(SEARCH_STR1)) == 0) {
        sizePos = f_tell(&Fil) - bytes_read + i;
        break;  // Pokud chcete pokračovat ve vyhledávání dalších výskytů, odstraňte tento 'break'.
      }
    }

    // Hledej "XYZI" v bufferu
    for (size_t i = 0; i <= bytes_read - strlen(SEARCH_STR2); i++) {
      if (memcmp(buffer + i, SEARCH_STR2, strlen(SEARCH_STR2)) == 0) {
        xyziPos = f_tell(&Fil) - bytes_read + i;
        break;  // Opět, pokud chcete najít všechny výskyty, můžete odstranit tento 'break'.
      }
    }
    for (size_t i = 0; i <= bytes_read - strlen(SEARCH_STR3); i++) {
      if (memcmp(buffer + i, SEARCH_STR3, strlen(SEARCH_STR3)) == 0) {
        rgbaPos = f_tell(&Fil) - bytes_read + i;
        break;  // Pokud chcete pokračovat ve vyhledávání dalších výskytů, odstraňte tento 'break'.
      }
    }

    // Pokud jste našli oba řetězce, můžete ukončit hledání
    if (sizePos > 0 && xyziPos > 0 && rgbaPos > 0) {
      break;
    }
  }

  if ((sizePos > 0) &&
    (f_lseek(&Fil, sizePos+12) == 0) &&
    (f_read(&Fil, buf, 12, &bytes_read) == FR_OK) &&
    (buf[0] == 8) && (buf[1] == 8) && (buf[1] == 8) &&
    (rgbaPos > 0) &&
    (f_lseek(&Fil, rgbaPos+4) == 0) &&
    (f_read(&Fil, &paletteSize, 4, &bytes_read) == FR_OK) &&
    (paletteSize == 1024) &&
    (f_lseek(&Fil, rgbaPos+8) == 0) &&
    (f_read(&Fil, palette, paletteSize, &bytes_read) == FR_OK) &&
    (xyziPos > 0) &&
    (f_lseek(&Fil, xyziPos+12) == 0) &&
    (f_read(&Fil, &voxelCount, 4, &bytes_read) == FR_OK) ) {
//    printf("Size %ldx%ldx%ld\n", buf[0], buf[1], buf[2]);
//    printf("paletteSize %ld\n", paletteSize);
//    printf("voxel count %ld\n", voxelCount);
    for(int i = 0; i < voxelCount; i++) {
      if (f_read(&Fil, &coord, 4, &bytes_read) == FR_OK) {
        memcpy(&scrBuf[coord.x][coord.y][coord.z], palette + coord.i * 4, sizeof(Voxel_t));
      }
    }
  }

  f_close(&Fil);
}


int logical_xor(int a, int b) {
  return (a || b) && !(a && b);
}

void App_ScreenToPixels()
{
  for(int z=0; z<8; z++) {
    for(int y=0; y<8; y++) {
      for(int x=0; x<8; x++) {
        Voxel_t vox = scrBuf[(logical_xor(y&1, !(z&2))) ? 7-x : x][(z&2) ? y : 7-y][z];
        CubeMux_SetPixel_Voxel(z/2, (z&1)*64 + y*8 + x, vox);
      }
    }
  }
}

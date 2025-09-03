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

char **filelist;
uint32_t pos = 0;
char *ptr;


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
  uint32_t pos = 0;
//  ptr = filelist[pos];
  CubeMux_StartMux();
}

void App_Handler()
{
//  if(Encoder_GetRelativePosition()!=0) {
//    ptr = filelist[++pos];
//    if (ptr == NULL) {
//      pos = 0;
//      ptr = filelist[pos];
//    }
//    ssd1306_Fill(Black);
//    ssd1306_SetCursor(1, 10);
//    ssd1306_WriteString(ptr, Font_7x10, White);
//    ssd1306_UpdateScreen();
//  }
	HAL_Delay(1);
}

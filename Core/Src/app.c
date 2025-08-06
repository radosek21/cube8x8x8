/*
 * app.c
 *
 *  Created on: Jun 24, 2025
 *      Author: radek.vanhara
 */
#include "app.h"
#include "main.h"
#include "ws28xx.h"


extern TIM_HandleTypeDef htim1;

#define LEDS_PAGES_CNT 4
WS28XX_HandleTypeDef hLeds[LEDS_PAGES_CNT];
const uint8_t tim1Channels[LEDS_PAGES_CNT] = {
  TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};
const uint16_t colors[] = {COLOR_RGB565_RED, COLOR_RGB565_GREEN, COLOR_RGB565_BLUE, COLOR_RGB565_WHITE};
int iter = 0;

static uint16_t screen[8][8];

void App_ScreenToPixels(WS28XX_HandleTypeDef *hLed, uint16_t scr[8][8]);

void App_Init()
{
  for(int i=0; i<LEDS_PAGES_CNT; i++) {
    WS28XX_Init(&hLeds[i], &htim1, tim1Channels[i], WS28XX_PIXEL_MAX);
    WS28XX_SetPixel_RGBW_565(&hLeds[i], 0, COLOR_RGB565_BLACK, 0);
    WS28XX_Update(&hLeds[i]);
  }

}

void App_Handler()
{
	static bool dirX, dirY = false;
	static int ix, iy = 0;
	if(!dirX) {
		ix++;
		if(ix==7) {
			dirX = true;
		}
	} else {
		ix--;
		if(ix==0) {
			dirX = false;
		}
	}
	if(iter%3 == 0) {
		if(!dirY) {
			iy++;
			if(iy==7) {
				dirY = true;
			}
		} else {
			iy--;
			if(iy==0) {
				dirY = false;
			}
		}
	}
	for(int x=0; x<8; x++) {
		  for(int y=0; y<8; y++) {
			  screen[x][y] = (x==ix && y==iy) ? COLOR_RGB565_BLUE : COLOR_RGB565_BLACK;
		  }
	  }
	  App_ScreenToPixels(&hLeds[1], screen);
	  WS28XX_Update(&hLeds[1]);

	iter++;
  HAL_Delay(50);

}

void App_ScreenToPixels(WS28XX_HandleTypeDef *hLed, uint16_t scr[8][8])
{
	for(int y=0; y<8; y++) {
		for(int x=0; x<8; x++) {
			WS28XX_SetPixel_RGBW_565(hLed, y*8+x, scr[(y&1) ? x : 7-x][y], 63);
		}
	}
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{

}


/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "cube_mux.h"
#include <string.h>

/************************************************************************************************************
**************    Private Functions
************************************************************************************************************/


#define LEDS_PAGES_CNT 4

typedef struct {
  WS28XX_HandleTypeDef hLeds[LEDS_PAGES_CNT];
  uint16_t screen[8][8];
  uint8_t blackScreenBuffer[(WS28XX_PIXEL_MAX * 24) + SW28xx_DMA_DUMMY_BYTES];
  uint16_t currMuxId;
  uint32_t muxPendingMask;
  uint32_t muxUpdateMask;
} CubeMux_t;

typedef struct {
  uint32_t timChannel;
  uint32_t dmaChannel;
  uint32_t ccr;
} MuxDmaConf_t;


static CubeMux_t cubeMux;
static const MuxDmaConf_t muxDmaConf[LEDS_PAGES_CNT] = {
    {LL_TIM_CHANNEL_CH1, LL_DMA_CHANNEL_2, (uint32_t)&(TIM1->CCR1)},
    {LL_TIM_CHANNEL_CH2, LL_DMA_CHANNEL_3, (uint32_t)&(TIM1->CCR2)},
    {LL_TIM_CHANNEL_CH3, LL_DMA_CHANNEL_7, (uint32_t)&(TIM1->CCR3)},
    {LL_TIM_CHANNEL_CH4, LL_DMA_CHANNEL_4, (uint32_t)&(TIM1->CCR4)},
};


static void CubeMux_NextMux();

void CubeMux_Init()
{
  WS28XX_HandleTypeDef *hLed;
  for(int i=0; i<LEDS_PAGES_CNT; i++) {
    hLed = &cubeMux.hLeds[i];
    WS28XX_Init(hLed, 1, WS28XX_PIXEL_MAX);
    for(int p = 0; p < WS28XX_PIXEL_MAX; p++) {
      WS28XX_SetPixel_RGB_565(hLed, p, COLOR_RGB565_BLACK);
    }
    WS28XX_Update(hLed);
    LL_TIM_CC_EnableChannel(TIM1, muxDmaConf[i].timChannel);
  }
  memset(cubeMux.blackScreenBuffer, 0, sizeof(cubeMux.blackScreenBuffer));
  memset(cubeMux.blackScreenBuffer, WS28XX_PULSE_0, WS28XX_PIXEL_MAX*24);
  LL_TIM_EnableDMAReq_CC1(TIM1);
  LL_TIM_EnableDMAReq_CC2(TIM1);
  LL_TIM_EnableDMAReq_CC3(TIM1);
  LL_TIM_EnableDMAReq_CC4(TIM1);

  LL_TIM_EnableAllOutputs(TIM1);

  cubeMux.currMuxId = 0;
  cubeMux.muxPendingMask = 0;
  cubeMux.muxUpdateMask = 0;
}

void CubeMux_StartMux()
{
  cubeMux.currMuxId = 0;
  cubeMux.muxUpdateMask = 0;
  CubeMux_NextMux();
}


static void CubeMux_NextMux()
{
  cubeMux.muxPendingMask = 0xF;

  LL_TIM_DisableCounter(TIM1);
  if (cubeMux.muxUpdateMask == 0xF) {
    cubeMux.muxUpdateMask = 0;
    for(int i=0; i<LEDS_PAGES_CNT; i++) {
      WS28XX_Update(&cubeMux.hLeds[i]);
    }
  }

  for(int i=0; i<LEDS_PAGES_CNT; i++) {
    LL_DMA_ConfigAddresses(DMA1, muxDmaConf[i].dmaChannel, (i == cubeMux.currMuxId) ? (uint32_t)cubeMux.hLeds[i].Buffer : (uint32_t)cubeMux.blackScreenBuffer, muxDmaConf[i].ccr, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, muxDmaConf[i].dmaChannel, (WS28XX_PIXEL_MAX * 24) + SW28xx_DMA_DUMMY_BYTES);
    LL_DMA_EnableChannel(DMA1, muxDmaConf[i].dmaChannel);
    LL_DMA_EnableIT_TC(DMA1, muxDmaConf[i].dmaChannel);
  }
  LL_TIM_EnableCounter(TIM1);

  cubeMux.currMuxId++;
  cubeMux.currMuxId %= LEDS_PAGES_CNT;
}

void CubeMux_SetPixel_Voxel(int hLedId, uint16_t Pixel, Voxel_t vox)
{
  cubeMux.muxUpdateMask |= (1 << hLedId);
  WS28XX_SetPixel_Voxel(&cubeMux.hLeds[hLedId], Pixel, vox);
}

void TIM_PWM_PulseFinished_Callback(uint32_t channel)
{
    cubeMux.muxPendingMask &= ~channel;
    if (cubeMux.muxPendingMask == 0) {
        CubeMux_NextMux();
    }
}

/***********************************************************************************************************/

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
#include "display.h"
#include "fatfs.h"
#include "graphics.h"
#include "animation.h"




char **filelist;
uint32_t pos = 0;
char *filename;
int anim_changed = 1;


void App_Init()
{
  CubeMux_Init();
  Sdcard_Init();
  Encoder_Init();
  Display_Init();

  CubeMux_StartMux();
}

void App_Handler()
{
  // Check for SDCard insertion
  if ((BSP_SD_IsDetected() == SD_PRESENT) && !Sdcard_IsMounted()) {
    // Card inserted
    HAL_Delay(500);
    Display_ShowSystemStatus(DISPLAY_STATUS_LOADING_STR);
    Sdcard_Mount();
    if (Sdcard_IsMounted()) {
      filelist= Sdcard_GetFilesList();
      pos = 0;
      Display_ShowSystemStatus(DISPLAY_STATUS_READY_STR);
    }
  }

  // Check for SDCard removing
  if ((BSP_SD_IsDetected() == SD_NOT_PRESENT) && Sdcard_IsMounted()) {
    // Card removed
    Display_ShowSystemStatus(DISPLAY_STATUS_NO_SD_STR);
    Sdcard_Unmount();
    FATFS_UnLinkDriver(SDPath);
    MX_FATFS_Init();
    filelist = NULL;
    pos = 0;
  }

  // Rotary encoder UI handling

  /*
  int16_t relPos = Encoder_GetRelativePosition();
  if (relPos != 0) {
    filename = filelist[++pos];
    if (filename == NULL) {
      pos = 0;
      filename = filelist[pos];
    }
    Display_ShowFilename(filename);
    Graphics_ShowVoxFile(filename);
  }
  */

  int16_t relPos = Encoder_GetRelativePosition();
  if (relPos > 0)
  {
    animation_select_next();
    Display_ShowFilename(animation_get_name());
  }
  else if (relPos < 0)
  {
    animation_select_previous();
    Display_ShowFilename(animation_get_name());
  }

  animation_show();


  HAL_Delay(1);
}


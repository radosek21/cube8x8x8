
#include "display.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

void Display_Init()
{
  ssd1306_Init();
  Display_ShowSystemStatus(DISPLAY_STATUS_NO_SD_STR);
}

void Display_ShowSystemStatus(char *str)
{
  ssd1306_Fill(Black);
  ssd1306_SetCursor(6, 12);
  ssd1306_WriteString(str, Font_11x18, White);
  ssd1306_UpdateScreen();
}


void Display_ShowFilename(char *filename)
{
  ssd1306_Fill(Black);
  ssd1306_SetCursor(1, 10);
  ssd1306_WriteString(filename, Font_7x10, White);
  ssd1306_UpdateScreen();
}


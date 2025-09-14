
#ifndef DISPLAY_H_
#define DISPLAY_H_

#define DISPLAY_STATUS_LOADING_STR  "Loading..."
#define DISPLAY_STATUS_NO_SD_STR    "No SD card."
#define DISPLAY_STATUS_READY_STR    "Ready"

void Display_Init();
void Display_ShowSystemStatus(char *str);
void Display_ShowFilename(char *filename);


#endif /* DISPLAY_H_ */


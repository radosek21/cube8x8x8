/*
 * app.h
 *
 *  Created on: Jun 24, 2025
 *      Author: radek.vanhara
 */

#ifndef SRC_SDCARD_H_
#define SRC_SDCARD_H_

#include <stdbool.h>

void Sdcard_Init();
void Sdcard_Mount();
void Sdcard_Unmount();
bool Sdcard_IsMounted();
void Sdcard_ScanFiles();
char** Sdcard_GetFilesList();


void SDIO_SDCard_Test(void);

#endif /* SRC_SDCARD_H_ */

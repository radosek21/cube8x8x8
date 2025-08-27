/*
 * app.h
 *
 *  Created on: Jun 24, 2025
 *      Author: radek.vanhara
 */

#ifndef SRC_APP_H_
#define SRC_APP_H_

void Sdcard_Init();
void Sdcard_Mount();
void Sdcard_Unmount();
void Sdcard_ScanFiles();
char** Sdcard_GetFilesList();


void SDIO_SDCard_Test(void);

#endif /* SRC_APP_H_ */

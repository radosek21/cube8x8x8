#ifndef _CUBE_MUX_H_
#define _CUBE_MUX_H_

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "ws28xx.h"




/************************************************************************************************************
**************    Public Functions
************************************************************************************************************/

void CubeMux_Init();
void CubeMux_StartMux();
void TIM_PWM_PulseFinished_Callback(uint32_t channel);
void CubeMux_SetPixel_Voxel(int hLedId, uint16_t Pixel, Voxel_t vox);


#ifdef __cplusplus
}
#endif

#endif // _CUBE_MUX_H_

/**
 * @file       encoder.h
 * @version    $(APP_VERSION)
 * @date       $(RELEASE_DATE)
 * @brief      Encoder driver
 * @author     jan.bartovsky
 * @author     ondrej.lufinka
 *
 * @copyright  Logic Elements Copyright
 *
 * @defgroup grEnc Encoder driver
 * @{
 * @brief group_brief
 *
 * This module contains
 *
 * @par Main features:
 *
 * @par Example
 * @code
 *
 * @endcode
 */

/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef ENCODER_H_
#define ENCODER_H_

/* Includes ----------------------------------------------------------------- */
#include "main.h"
/* Common includes */

/* Definitions--------------------------------------------------------------- */

/* BSP */
#define ENCODER_CLK_ENABLE()  do{                             \
                                __HAL_RCC_GPIOB_CLK_ENABLE(); \
                                __HAL_RCC_TIM4_CLK_ENABLE();  \
                              } while(0)                          ///< Enable encoder clocks

#define ENCODER_TIM_INSTANCE            TIM4                      ///< Encoder TIM instance
#define ENCODER_TIM_ALTERNATE           GPIO_AF2_TIM4             ///< Encoder TIM alternate

#define ENCODER_POLARITY                LL_TIM_IC_POLARITY_RISING     ///< Encoder polarity - rising edge
#define ENCODER_PULL                    GPIO_NOPULL               ///< Encoder pull resistor - no pull
#define ENCODER_LEFT_PORT               GPIOB                     ///< Encoder left GPIO PORT
#define ENCODER_LEFT_PIN                GPIO_PIN_6                ///< Encoder left GPIO PIN
#define ENCODER_RIGHT_PORT              GPIOB                     ///< Encoder right GPIO PORT
#define ENCODER_RIGHT_PIN               GPIO_PIN_7                ///< Encoder right GPIO PIN

#define ENCODER_IRQ_NUMBER              TIM4_IRQn                 ///< Encoder IRQ number
#define ENCODER_IRQ_HANDLER             TIM4_IRQHandler           ///< Encoder IRQ handler
#define PRIO_IRQ_ENCODER            	15      ///< Encoder timer

/* Typedefs------------------------------------------------------------------ */

/* Functions ---------------------------------------------------------------- */

/**
 * Initialize encoder module
 *
 * @param none
 * @return STATUS_OK - if init done
 * @return STATUS_ERROR - if error during init
 */
void Encoder_Init(void);

/**
 * Get absolute position of encoder
 *
 * @param none
 * @return absolute position of encoder
 */
int16_t Encoder_GetAbsolutePosition(void);

/**
 * Get relative position of encoder
 *
 * @param none
 * @return relative position of encoder
 */
int16_t Encoder_GetRelativePosition(void);


#endif /* ENCODER_H_ */

/** @} */

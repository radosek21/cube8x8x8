/**
 * @file       encoder.c
 * @brief      Encoder driver
 * @addtogroup grEnc
 * @{
 */

/* Includes ----------------------------------------------------------------- */

#include "encoder.h"

/* Application includes */

/* Private defines ---------------------------------------------------------- */

/* Private macros  ---------------------------------------------------------- */

/* Private typedefs --------------------------------------------------------- */

/**
 * Declaration of all private variables
 */
typedef struct
{
  uint8_t going_left;
  uint8_t going_right;

  uint16_t htim_instance_CNT;
  uint16_t htim_instance_CNT_last;
  uint16_t cnt;

  int16_t absolute_pos;
  int16_t relative_pos;
  int16_t relative_pos_last;
}encoder_Private_t;

/* Private constants -------------------------------------------------------- */

/* Private variables -------------------------------------------------------- */

/**
 * Instance of all private variables (except HAL handles)
 */
static encoder_Private_t enc;

static TIM_HandleTypeDef htimEnc;

/* Public variables --------------------------------------------------------- */

/* Private function prototypes ---------------------------------------------- */

static Status_t Encoder_HalInit(void);

/* Functions ---------------------------------------------------------------- */

void Encoder_Init(void)
{
  Encoder_HalInit();

  enc.going_left = 0;
  enc.going_right = 0;

  enc.htim_instance_CNT = 0;
  enc.htim_instance_CNT_last = 0;
  enc.cnt = 0;

  enc.absolute_pos = 0;
  enc.relative_pos = 0;
  enc.relative_pos_last = 0;

}

int16_t Encoder_GetAbsolutePosition(void)
{
  enc.absolute_pos = (int16_t)enc.cnt;
  return enc.absolute_pos;
}

int16_t Encoder_GetRelativePosition(void)
{
  enc.absolute_pos = (int16_t)enc.cnt;

  enc.relative_pos = enc.absolute_pos - enc.relative_pos_last;
  enc.relative_pos_last = enc.absolute_pos;

  return enc.relative_pos;
}

/* Private Functions -------------------------------------------------------- */

/**
* This function handles button 1 IRQ
*/
void ENCODER_IRQ_HANDLER(void)
{
  /* Capture compare 1 event */
  if (__HAL_TIM_GET_FLAG(&htimEnc, TIM_FLAG_CC1) != RESET)
  {
    if (__HAL_TIM_GET_IT_SOURCE(&htimEnc, TIM_IT_CC1) != RESET)
    {
      {
        __HAL_TIM_CLEAR_IT(&htimEnc, TIM_IT_CC1);

        enc.htim_instance_CNT = (uint16_t)htimEnc.Instance->CNT;

        if (enc.going_left && !enc.going_right &&
           (enc.htim_instance_CNT <= enc.htim_instance_CNT_last ||
           (enc.htim_instance_CNT_last < 4 && enc.htim_instance_CNT > UINT16_MAX - 4)))
        {
          enc.cnt--;
          enc.going_left = 0;
        }
        else if (enc.htim_instance_CNT >= enc.htim_instance_CNT_last ||
                (enc.htim_instance_CNT < 4 && enc.htim_instance_CNT_last > UINT16_MAX - 4))
        {
          enc.going_left = 0;
          enc.going_right = 1;
        }

        enc.htim_instance_CNT_last = enc.htim_instance_CNT;
      }
    }
  }

  /* Capture compare 2 event */
  if (__HAL_TIM_GET_FLAG(&htimEnc, TIM_FLAG_CC2) != RESET)
  {
    if (__HAL_TIM_GET_IT_SOURCE(&htimEnc, TIM_IT_CC2) != RESET)
    {
      {
        __HAL_TIM_CLEAR_IT(&htimEnc, TIM_IT_CC2);

        enc.htim_instance_CNT = (uint16_t)htimEnc.Instance->CNT;


        if (!enc.going_left && enc.going_right &&
           (enc.htim_instance_CNT >= enc.htim_instance_CNT_last ||
           (enc.htim_instance_CNT < 4 && enc.htim_instance_CNT_last > UINT16_MAX - 4)))
        {
          enc.cnt++;
          enc.going_right = 0;
        }

        else if (enc.htim_instance_CNT <= enc.htim_instance_CNT_last ||
                (enc.htim_instance_CNT_last < 4 && enc.htim_instance_CNT > UINT16_MAX - 4))
        {
          enc.going_left = 1;
          enc.going_right = 0;
        }
        enc.htim_instance_CNT_last = enc.htim_instance_CNT;
      }
    }
  }

  /* Capture compare 3 event */
  if (__HAL_TIM_GET_FLAG(&htimEnc, TIM_FLAG_CC3) != RESET)
  {
    if (__HAL_TIM_GET_IT_SOURCE(&htimEnc, TIM_IT_CC3) != RESET)
    {
      {
        __HAL_TIM_CLEAR_IT(&htimEnc, TIM_IT_CC3);

        // Do nothing
      }
    }
  }

  /* Capture compare 4 event */
  if (__HAL_TIM_GET_FLAG(&htimEnc, TIM_FLAG_CC4) != RESET)
  {
    if (__HAL_TIM_GET_IT_SOURCE(&htimEnc, TIM_IT_CC4) != RESET)
    {
      {
        __HAL_TIM_CLEAR_IT(&htimEnc, TIM_IT_CC4);

        // Do nothing
      }
    }
  }
}

/**
* This function initializes HAL for encoder
*/
static Status_t Encoder_HalInit(void)
{
  Status_t status = STATUS_OK;

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* Peripheral clock enable */
  ENCODER_CLK_ENABLE();

  htimEnc.Instance = ENCODER_TIM_INSTANCE;
  htimEnc.Init.Prescaler = 1;
  htimEnc.Init.CounterMode = TIM_COUNTERMODE_UP;
  htimEnc.Init.Period = UINT16_MAX;
  htimEnc.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htimEnc.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = ENCODER_POLARITY;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 15;
  sConfig.IC2Polarity = ENCODER_POLARITY;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 15;
  if (HAL_TIM_Encoder_Init(&htimEnc, &sConfig) != HAL_OK)
  {
    status |= (STATUS_ERROR | STATUS_FAIL);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htimEnc, &sMasterConfig) != HAL_OK)
  {
    status |= (STATUS_ERROR | STATUS_FAIL);
  }

  /* GPIO Configuration */
  GPIO_InitStruct.Pin = ENCODER_LEFT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = ENCODER_PULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = ENCODER_TIM_ALTERNATE;
  HAL_GPIO_Init(ENCODER_LEFT_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = ENCODER_RIGHT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = ENCODER_PULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = ENCODER_TIM_ALTERNATE;
  HAL_GPIO_Init(ENCODER_RIGHT_PORT, &GPIO_InitStruct);

  /* Enable and set encoder interrupt to its priority */
  HAL_NVIC_SetPriority(ENCODER_IRQ_NUMBER, PRIO_IRQ_ENCODER, 0x00);
  HAL_NVIC_ClearPendingIRQ(ENCODER_IRQ_NUMBER);
  HAL_NVIC_EnableIRQ(ENCODER_IRQ_NUMBER);

  /* Start timer */
  if (HAL_TIM_Encoder_Start_IT(&htimEnc, TIM_CHANNEL_ALL) != HAL_OK)
  {
    status |= (STATUS_ERROR | STATUS_FAIL);
  }

  return status;
}

/** @} */

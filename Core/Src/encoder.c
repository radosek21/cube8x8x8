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

  uint16_t button_shift_reg;
  uint32_t button_timer;
  uint16_t button_pressed;
  uint16_t button_event;

}encoder_Private_t;

/* Private constants -------------------------------------------------------- */

/* Private variables -------------------------------------------------------- */

/**
 * Instance of all private variables (except HAL handles)
 */
static encoder_Private_t enc;


/* Public variables --------------------------------------------------------- */

/* Private function prototypes ---------------------------------------------- */

static Status_t Encoder_LL_Init(void);

/* Functions ---------------------------------------------------------------- */

void Encoder_Init(void)
{
  Encoder_LL_Init();

  enc.going_left = 0;
  enc.going_right = 0;

  enc.htim_instance_CNT = 0;
  enc.htim_instance_CNT_last = 0;
  enc.cnt = 0;

  enc.absolute_pos = 0;
  enc.relative_pos = 0;
  enc.relative_pos_last = 0;

}

void Encoder_ButtonPoll(void)
{
  if (TICK_EXPIRED(enc.button_timer))
  {
    enc.button_timer = HAL_GetTick() + 5;

    enc.button_shift_reg <<= 1;
    if (!(LL_GPIO_ReadInputPort(ENCODER_BUTTON_PORT) & (ENCODER_BUTTON_PIN)))
    {
      enc.button_shift_reg |= 1;
    }

    if ((enc.button_shift_reg & 0xFF) == 0xFF)
    {
      enc.button_pressed = 1;
    }
    else if ((enc.button_shift_reg & 0xFF) == 0x00)
    {
      if (enc.button_pressed)
      {
        enc.button_pressed = 0;
        enc.button_event = 1;
      }
    }
  }
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

uint16_t Encoder_GetButtonPressed(void)
{
  uint16_t temp = enc.button_event;
  enc.button_event = 0;
  return temp;
}

/* Private Functions -------------------------------------------------------- */

/**
* This function handles button 1 IRQ
*/
void ENCODER_IRQ_HANDLER(void)
{
    /* Capture compare 1 event */
    if (LL_TIM_IsActiveFlag_CC1(ENCODER_TIM_INSTANCE) && LL_TIM_IsEnabledIT_CC1(ENCODER_TIM_INSTANCE))
    {
        LL_TIM_ClearFlag_CC1(ENCODER_TIM_INSTANCE);

        enc.htim_instance_CNT = (uint16_t)LL_TIM_GetCounter(ENCODER_TIM_INSTANCE);

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

    /* Capture compare 2 event */
    if (LL_TIM_IsActiveFlag_CC2(ENCODER_TIM_INSTANCE) && LL_TIM_IsEnabledIT_CC2(ENCODER_TIM_INSTANCE))
    {
        LL_TIM_ClearFlag_CC2(ENCODER_TIM_INSTANCE);

        enc.htim_instance_CNT = (uint16_t)LL_TIM_GetCounter(ENCODER_TIM_INSTANCE);

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

    /* Capture compare 3 event */
    if (LL_TIM_IsActiveFlag_CC3(ENCODER_TIM_INSTANCE) && LL_TIM_IsEnabledIT_CC3(ENCODER_TIM_INSTANCE))
    {
        LL_TIM_ClearFlag_CC3(ENCODER_TIM_INSTANCE);

        // Do nothing
    }

    /* Capture compare 4 event */
    if (LL_TIM_IsActiveFlag_CC4(ENCODER_TIM_INSTANCE) && LL_TIM_IsEnabledIT_CC4(ENCODER_TIM_INSTANCE))
    {
        LL_TIM_ClearFlag_CC4(ENCODER_TIM_INSTANCE);

        // Do nothing
    }
    if (LL_TIM_IsActiveFlag_UPDATE(ENCODER_TIM_INSTANCE)) {
      LL_TIM_ClearFlag_UPDATE(ENCODER_TIM_INSTANCE);
            // Do nothing
    }
}

/**
* This function initializes HAL for encoder
*/
static Status_t Encoder_LL_Init(void)
{
    Status_t status = STATUS_OK;

    // Peripheral clock enable
    ENCODER_CLK_ENABLE();

    // Reset and configure the TIM instance
    LL_TIM_InitTypeDef TIM_InitStruct = {0};
    LL_TIM_ENCODER_InitTypeDef encoderInitStruct = {0};

    // GPIO Configuration
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = ENCODER_BUTTON_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    LL_GPIO_Init(ENCODER_BUTTON_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ENCODER_LEFT_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Pull = ENCODER_PULL;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = ENCODER_TIM_ALTERNATE;
    LL_GPIO_Init(ENCODER_LEFT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ENCODER_RIGHT_PIN;
    LL_GPIO_Init(ENCODER_RIGHT_PORT, &GPIO_InitStruct);

    // Configure TIM encoder interface
    TIM_InitStruct.Prescaler = 1;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = UINT16_MAX;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(ENCODER_TIM_INSTANCE, &TIM_InitStruct);

    encoderInitStruct.EncoderMode = LL_TIM_ENCODERMODE_X4_TI12;
    encoderInitStruct.IC1Polarity = ENCODER_POLARITY;
    encoderInitStruct.IC1ActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
    encoderInitStruct.IC1Prescaler = LL_TIM_ICPSC_DIV1;
    encoderInitStruct.IC1Filter = LL_TIM_IC_FILTER_FDIV16_N5;
    encoderInitStruct.IC2Polarity = ENCODER_POLARITY;
    encoderInitStruct.IC2ActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
    encoderInitStruct.IC2Prescaler = LL_TIM_ICPSC_DIV1;
    encoderInitStruct.IC2Filter = LL_TIM_IC_FILTER_FDIV16_N5;
    if (LL_TIM_ENCODER_Init(ENCODER_TIM_INSTANCE, &encoderInitStruct) != SUCCESS)
    {
        status |= (STATUS_ERROR | STATUS_FAIL);
    }

    // Master configuration
    LL_TIM_DisableMasterSlaveMode(ENCODER_TIM_INSTANCE);
    LL_TIM_SetTriggerOutput(ENCODER_TIM_INSTANCE, LL_TIM_TRGO_RESET);

    // Enable TIM counter
    LL_TIM_EnableCounter(ENCODER_TIM_INSTANCE);

    // NVIC configuration for TIM interrupt
    NVIC_SetPriority(ENCODER_IRQ_NUMBER, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), PRIO_IRQ_ENCODER, 0x00));
    NVIC_ClearPendingIRQ(ENCODER_IRQ_NUMBER);
    NVIC_EnableIRQ(ENCODER_IRQ_NUMBER);

    // Enable update interrupt
    LL_TIM_EnableIT_CC1(ENCODER_TIM_INSTANCE);
    LL_TIM_EnableIT_CC2(ENCODER_TIM_INSTANCE);

    return status;
}

/** @} */

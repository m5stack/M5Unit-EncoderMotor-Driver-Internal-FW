#include "mymotorencoder.h"
#include "tim.h"
#include "sys_utils.h"

// int32_t encoder_raw[0],encoder_raw[1],encoder_raw[2],encoder_raw[3];
volatile encoder_raw_t encoder_raw[4] = {0};

int32_t GetEncoderPos(uint8_t port)
{
    int32_t pos = 0;
    switch (port) {
        case ENC_M1: {
            pos = encoder_raw[0].encoder_raw.value;
            break;
        }
        case ENC_M2: {
            pos = encoder_raw[1].encoder_raw.value;
            break;
        }
        case ENC_M3: {
            pos = encoder_raw[2].encoder_raw.value;
            break;
        }
        case ENC_M4: {
            pos = encoder_raw[3].encoder_raw.value;
            break;
        }
    }
    return pos;
}

void SetEncoderPos(uint8_t port, int32_t pos)
{
    switch (port) {
        case ENC_M1: {
            encoder_raw[0].encoder_raw.value = pos;
            break;
        }
        case ENC_M2: {
            encoder_raw[1].encoder_raw.value = pos;
            break;
        }
        case ENC_M3: {
            encoder_raw[2].encoder_raw.value = pos;
            break;
        }
        case ENC_M4: {
            encoder_raw[3].encoder_raw.value = pos;
            break;
        }
    }
}
void ResetEncoderPos(uint8_t port)
{
    switch (port) {
        case ENC_M1: {
            encoder_raw[0].encoder_raw.value = 0;
            break;
        }
        case ENC_M2: {
            encoder_raw[1].encoder_raw.value = 0;
            break;
        }
        case ENC_M3: {
            encoder_raw[2].encoder_raw.value = 0;
            break;
        }
        case ENC_M4: {
            encoder_raw[3].encoder_raw.value = 0;
            break;
        }
    }
}

void InitMotorEncoder(void)
{
    ResetEncoderPos(ENC_M1);
    ResetEncoderPos(ENC_M2);
    ResetEncoderPos(ENC_M3);
    ResetEncoderPos(ENC_M4);
}

void InterruptMotorEncoder(void)
{
    volatile uint32_t gpioa_buf = GPIOA->IDR;
    volatile uint32_t gpiob_buf = GPIOB->IDR;

    if (!motor_data.encoder_ab_mode) {
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M1A_Pin) != RESET)  // M1, PA0+PA1
        {
            if ((gpioa_buf & MCU_ENC_M1B_Pin) ^ ((gpioa_buf & MCU_ENC_M1A_Pin) << 1)) {
                encoder_raw[0].encoder_raw.value += 1;
            } else {
                encoder_raw[0].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M1A_Pin);
        }

        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M1B_Pin) != RESET) {
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M1B_Pin);
        }
    } else {
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M1B_Pin) != RESET)  // M1, PA1
        {
            if ((gpioa_buf & MCU_ENC_M1A_Pin) ^ ((gpioa_buf & MCU_ENC_M1B_Pin) >> 1)) {
                encoder_raw[0].encoder_raw.value += 1;
            } else {
                encoder_raw[0].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M1B_Pin);
        }

        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M1A_Pin) != RESET) {
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M1A_Pin);
        }
    }
}

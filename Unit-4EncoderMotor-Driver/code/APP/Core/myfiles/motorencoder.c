#include "../myfiles/motorencoder.h"
#include "tim.h"
#include "sys_utils.h"

volatile encoder_raw_t encoder_raw[4] = {0};

int32_t GetEncoderPos(uint8_t port)
{
    int32_t pos;
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
        // Motor 1 encoder interrupt - PA6 (A) + PA7 (B)
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M1A_Pin) != RESET) {
            if ((gpioa_buf & MCU_ENC_M1B_Pin) ^ ((gpioa_buf & MCU_ENC_M1A_Pin) << 1)) {
                encoder_raw[0].encoder_raw.value += 1;
            } else {
                encoder_raw[0].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M1A_Pin);
        }

        // Motor 2 encoder interrupt - PA4 (A) + PA5 (B)
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M2A_Pin) != RESET) {
            if ((gpioa_buf & MCU_ENC_M2B_Pin) ^ ((gpioa_buf & MCU_ENC_M2A_Pin) << 1)) {
                encoder_raw[1].encoder_raw.value += 1;
            } else {
                encoder_raw[1].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M2A_Pin);
        }

        // Motor 3 encoder interrupt - PB7 (A) + PB6 (B)
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M3A_Pin) != RESET) {
            if ((gpiob_buf & MCU_ENC_M3B_Pin) ^ ((gpiob_buf & MCU_ENC_M3A_Pin) >> 1)) {
                encoder_raw[2].encoder_raw.value += 1;
            } else {
                encoder_raw[2].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M3A_Pin);
        }

        // Motor 4 encoder interrupt - PB9 (A) + PB8 (B)
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M4A_Pin) != RESET) {
            if ((gpiob_buf & MCU_ENC_M4B_Pin) ^ ((gpiob_buf & MCU_ENC_M4A_Pin) >> 1)) {
                encoder_raw[3].encoder_raw.value += 1;
            } else {
                encoder_raw[3].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M4A_Pin);
        }
    } else {
        // Alternative mode - interrupt on B pins
        // Motor 1 encoder interrupt - PA7 (B)
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M1B_Pin) != RESET) {
            if (((gpioa_buf & MCU_ENC_M1A_Pin) << 1) ^ (gpioa_buf & MCU_ENC_M1B_Pin)) {
                encoder_raw[0].encoder_raw.value += 1;
            } else {
                encoder_raw[0].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M1B_Pin);
        }

        // Motor 2 encoder interrupt - PA5 (B)
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M2B_Pin) != RESET) {
            if (((gpioa_buf & MCU_ENC_M2A_Pin) << 1) ^ (gpioa_buf & MCU_ENC_M2B_Pin)) {
                encoder_raw[1].encoder_raw.value += 1;
            } else {
                encoder_raw[1].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M2B_Pin);
        }

        // Motor 3 encoder interrupt - PB6 (B)
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M3B_Pin) != RESET) {
            if (((gpiob_buf & MCU_ENC_M3A_Pin) >> 1) ^ (gpiob_buf & MCU_ENC_M3B_Pin)) {
                encoder_raw[2].encoder_raw.value += 1;
            } else {
                encoder_raw[2].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M3B_Pin);
        }

        // Motor 4 encoder interrupt - PB8 (B)
        if (__HAL_GPIO_EXTI_GET_IT(MCU_ENC_M4B_Pin) != RESET) {
            if (((gpiob_buf & MCU_ENC_M4A_Pin) >> 1) ^ (gpiob_buf & MCU_ENC_M4B_Pin)) {
                encoder_raw[3].encoder_raw.value += 1;
            } else {
                encoder_raw[3].encoder_raw.value -= 1;
            }
            __HAL_GPIO_EXTI_CLEAR_IT(MCU_ENC_M4B_Pin);
        }
    }
}

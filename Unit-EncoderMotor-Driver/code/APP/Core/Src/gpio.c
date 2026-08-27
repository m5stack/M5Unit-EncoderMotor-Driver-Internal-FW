/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    gpio.c
 * @brief   This file provides code for the configuration
 *          of all used GPIO pins.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "sys_utils.h"

#define ENCODER_EXTI_LINES (LL_EXTI_LINE_0 | LL_EXTI_LINE_1)

static void encoder_exti_reset_lines(void)
{
    NVIC_DisableIRQ(EXTI0_1_IRQn);

    LL_EXTI_DisableIT_0_31(ENCODER_EXTI_LINES);
    LL_EXTI_DisableEvent_0_31(ENCODER_EXTI_LINES);
    LL_EXTI_DisableRisingTrig_0_31(ENCODER_EXTI_LINES);
    LL_EXTI_DisableFallingTrig_0_31(ENCODER_EXTI_LINES);
    LL_EXTI_ClearRisingFlag_0_31(ENCODER_EXTI_LINES);
    LL_EXTI_ClearFallingFlag_0_31(ENCODER_EXTI_LINES);
}

static void encoder_exti_enable_line(uint32_t exti_line)
{
    LL_EXTI_EnableRisingTrig_0_31(exti_line);
    LL_EXTI_EnableFallingTrig_0_31(exti_line);
    LL_EXTI_ClearRisingFlag_0_31(exti_line);
    LL_EXTI_ClearFallingFlag_0_31(exti_line);
    LL_EXTI_EnableIT_0_31(exti_line);
}
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
 */
void MX_GPIO_Init(void)
{
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M1R_GPIO_Port, MCU_DIR_M1R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M1F_GPIO_Port, MCU_DIR_M1F_Pin);

    /**/
    LL_GPIO_ResetOutputPin(DRV_NSLEEP_GPIO_Port, DRV_NSLEEP_Pin);

    /**/
    LL_GPIO_SetOutputPin(DC_EN_GPIO_Port, DC_EN_Pin);  // 上电使能DC引脚，启用VM外部供电
                                                       //  LL_GPIO_ResetOutputPin(DC_EN_GPIO_Port, DC_EN_Pin);

    /**/
    LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE0);

    /**/
    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_0;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /**/
    LL_GPIO_SetPinPull(MCU_ENC_M1A_GPIO_Port, MCU_ENC_M1A_Pin, LL_GPIO_PULL_UP);

    /**/
    LL_GPIO_SetPinMode(MCU_ENC_M1A_GPIO_Port, MCU_ENC_M1A_Pin, LL_GPIO_MODE_INPUT);

    /**/
    GPIO_InitStruct.Pin  = MCU_ENC_M1B_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_ENC_M1B_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M1R_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M1R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M1F_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M1F_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = DRV_NSLEEP_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(DRV_NSLEEP_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = DC_EN_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(DC_EN_GPIO_Port, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI0_1_IRQn, 0);
    NVIC_EnableIRQ(EXTI0_1_IRQn);
}

/* USER CODE BEGIN 2 */
void user_gpio_init(void)
{
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    encoder_exti_reset_lines();

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M1R_GPIO_Port, MCU_DIR_M1R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M1F_GPIO_Port, MCU_DIR_M1F_Pin);

    /**/
    LL_GPIO_ResetOutputPin(DRV_NSLEEP_GPIO_Port, DRV_NSLEEP_Pin);

    /**/
    LL_GPIO_SetOutputPin(DC_EN_GPIO_Port, DC_EN_Pin);  // 上电使能DC引脚，启用VM外部供电
    //  LL_GPIO_ResetOutputPin(DC_EN_GPIO_Port, DC_EN_Pin);		// 上电给DC低电平，禁用VM外部供电

    /**/
    if (!motor_data.encoder_ab_mode) {
        /**/
        LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE0);

        EXTI_InitStruct.Line_0_31   = ENCODER_EXTI_LINES;
        EXTI_InitStruct.LineCommand = DISABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        encoder_exti_enable_line(LL_EXTI_LINE_0);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M1A_GPIO_Port, MCU_ENC_M1A_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M1A_GPIO_Port, MCU_ENC_M1A_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M1B_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M1B_GPIO_Port, &GPIO_InitStruct);
    } else {
        /**/
        LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE1);

        EXTI_InitStruct.Line_0_31   = ENCODER_EXTI_LINES;
        EXTI_InitStruct.LineCommand = DISABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        encoder_exti_enable_line(LL_EXTI_LINE_1);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M1B_GPIO_Port, MCU_ENC_M1B_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M1B_GPIO_Port, MCU_ENC_M1B_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M1A_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M1A_GPIO_Port, &GPIO_InitStruct);
    }

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M1R_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M1R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M1F_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M1F_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = DRV_NSLEEP_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(DRV_NSLEEP_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = DC_EN_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(DC_EN_GPIO_Port, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI0_1_IRQn, 0);
    NVIC_EnableIRQ(EXTI0_1_IRQn);
}
/* USER CODE END 2 */

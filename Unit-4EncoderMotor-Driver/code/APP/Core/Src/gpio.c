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
#include "../myfiles/sys_utils.h"
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
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

    /**/
    LL_GPIO_ResetOutputPin(RGB_GPIO_Port, RGB_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M2R_GPIO_Port, MCU_DIR_M2R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M2F_GPIO_Port, MCU_DIR_M2F_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M1R_GPIO_Port, MCU_DIR_M1R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M1F_GPIO_Port, MCU_DIR_M1F_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M3R_GPIO_Port, MCU_DIR_M3R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M3F_GPIO_Port, MCU_DIR_M3F_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M4R_GPIO_Port, MCU_DIR_M4R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M4F_GPIO_Port, MCU_DIR_M4F_Pin);

    /**/
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE4);

    /**/
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE6);

    /**/
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE7);

    /**/
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE9);

    /**/
    LL_GPIO_SetPinPull(MCU_ENC_M2A_GPIO_Port, MCU_ENC_M2A_Pin, LL_GPIO_PULL_UP);

    /**/
    LL_GPIO_SetPinPull(MCU_ENC_M1A_GPIO_Port, MCU_ENC_M1A_Pin, LL_GPIO_PULL_NO);

    /**/
    LL_GPIO_SetPinPull(MCU_ENC_M3A_GPIO_Port, MCU_ENC_M3A_Pin, LL_GPIO_PULL_UP);

    /**/
    LL_GPIO_SetPinPull(MCU_ENC_M4A_GPIO_Port, MCU_ENC_M4A_Pin, LL_GPIO_PULL_UP);

    /**/
    LL_GPIO_SetPinMode(MCU_ENC_M2A_GPIO_Port, MCU_ENC_M2A_Pin, LL_GPIO_MODE_INPUT);

    /**/
    LL_GPIO_SetPinMode(MCU_ENC_M1A_GPIO_Port, MCU_ENC_M1A_Pin, LL_GPIO_MODE_INPUT);

    /**/
    LL_GPIO_SetPinMode(MCU_ENC_M3A_GPIO_Port, MCU_ENC_M3A_Pin, LL_GPIO_MODE_INPUT);

    /**/
    LL_GPIO_SetPinMode(MCU_ENC_M4A_GPIO_Port, MCU_ENC_M4A_Pin, LL_GPIO_MODE_INPUT);

    /**/
    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_4;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /**/
    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_6;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /**/
    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_7;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /**/
    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_9;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /**/
    GPIO_InitStruct.Pin  = MCU_ENC_M2B_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_ENC_M2B_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin  = MCU_ENC_M1B_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_ENC_M1B_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = RGB_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(RGB_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M2R_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M2R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M2F_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M2F_GPIO_Port, &GPIO_InitStruct);

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
    GPIO_InitStruct.Pin        = MCU_DIR_M3R_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M3R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M3F_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M3F_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M4R_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M4R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M4F_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M4F_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin  = MCU_ENC_M3B_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_ENC_M3B_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin  = MCU_ENC_M4B_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_ENC_M4B_GPIO_Port, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI4_15_IRQn, 1);
    NVIC_EnableIRQ(EXTI4_15_IRQn);
}

/* USER CODE BEGIN 2 */
void user_gpio_init(void)
{
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

    /**/
    LL_GPIO_ResetOutputPin(RGB_GPIO_Port, RGB_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M2R_GPIO_Port, MCU_DIR_M2R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M2F_GPIO_Port, MCU_DIR_M2F_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M1R_GPIO_Port, MCU_DIR_M1R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M1F_GPIO_Port, MCU_DIR_M1F_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M3R_GPIO_Port, MCU_DIR_M3R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M3F_GPIO_Port, MCU_DIR_M3F_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M4R_GPIO_Port, MCU_DIR_M4R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(MCU_DIR_M4F_GPIO_Port, MCU_DIR_M4F_Pin);

    if (!motor_data.encoder_ab_mode) {
        // config MOTOR_1 : MCU_ENC_M1A_Pin, MCU_ENC_M1B_Pin
        LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE6);

        EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_6;
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

        // config MOTOR_2 : MCU_ENC_M2A_Pin, MCU_ENC_M2B_Pin
        LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE4);

        EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_4;
        EXTI_InitStruct.LineCommand = ENABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M2A_GPIO_Port, MCU_ENC_M2A_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M2A_GPIO_Port, MCU_ENC_M2A_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M2B_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M2B_GPIO_Port, &GPIO_InitStruct);

        // config MOTOR_3 : MCU_ENC_M3A_Pin, MCU_ENC_M3B_Pin
        LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE7);

        EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_7;
        EXTI_InitStruct.LineCommand = ENABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M3A_GPIO_Port, MCU_ENC_M3A_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M3A_GPIO_Port, MCU_ENC_M3A_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M3B_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M3B_GPIO_Port, &GPIO_InitStruct);

        // config MOTOR_4 : MCU_ENC_M4A_Pin, MCU_ENC_M4B_Pin
        LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE9);

        EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_9;
        EXTI_InitStruct.LineCommand = ENABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M4A_GPIO_Port, MCU_ENC_M4A_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M4A_GPIO_Port, MCU_ENC_M4A_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M4B_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M4B_GPIO_Port, &GPIO_InitStruct);

    } else {
        // config MOTOR_1 : MCU_ENC_M1B_Pin, MCU_ENC_M1A_Pin
        LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE7);

        EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_7;
        EXTI_InitStruct.LineCommand = ENABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M1B_GPIO_Port, MCU_ENC_M1B_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M1B_GPIO_Port, MCU_ENC_M1B_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M1A_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M1A_GPIO_Port, &GPIO_InitStruct);

        // config MOTOR_2 : MCU_ENC_M2B_Pin, MCU_ENC_M2A_Pin
        LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE5);

        EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_5;
        EXTI_InitStruct.LineCommand = ENABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M2B_GPIO_Port, MCU_ENC_M2B_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M2B_GPIO_Port, MCU_ENC_M2B_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M2A_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M2A_GPIO_Port, &GPIO_InitStruct);

        // config MOTOR_3 : MCU_ENC_M3B_Pin, MCU_ENC_M3A_Pin
        LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE6);

        EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_6;
        EXTI_InitStruct.LineCommand = ENABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M3B_GPIO_Port, MCU_ENC_M3B_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M3B_GPIO_Port, MCU_ENC_M3B_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M3A_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M3A_GPIO_Port, &GPIO_InitStruct);

        // config MOTOR_4 : MCU_ENC_M4B_Pin, MCU_ENC_M4A_Pin
        LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE8);

        EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_8;
        EXTI_InitStruct.LineCommand = ENABLE;
        EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
        EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
        LL_EXTI_Init(&EXTI_InitStruct);

        /**/
        LL_GPIO_SetPinPull(MCU_ENC_M4B_GPIO_Port, MCU_ENC_M4B_Pin, LL_GPIO_PULL_UP);

        /**/
        LL_GPIO_SetPinMode(MCU_ENC_M4B_GPIO_Port, MCU_ENC_M4B_Pin, LL_GPIO_MODE_INPUT);

        /**/
        GPIO_InitStruct.Pin  = MCU_ENC_M4A_Pin;
        GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(MCU_ENC_M4A_GPIO_Port, &GPIO_InitStruct);
    }
    /**/
    GPIO_InitStruct.Pin        = RGB_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(RGB_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M2R_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M2R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M2F_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M2F_GPIO_Port, &GPIO_InitStruct);

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
    GPIO_InitStruct.Pin        = MCU_DIR_M3R_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M3R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M3F_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M3F_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M4R_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M4R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = MCU_DIR_M4F_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(MCU_DIR_M4F_GPIO_Port, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI4_15_IRQn, 1);
    NVIC_EnableIRQ(EXTI4_15_IRQn);
}
/* USER CODE END 2 */

/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

#include "stm32f0xx_ll_i2c.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_dma.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stm32f0xx_ll_i2c.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_dma.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef union {
    struct {
        uint8_t flash_err : 1;  // flash设置错误
        uint8_t adc_setup : 1;  // flash设置错误
        uint8_t rsvd : 6;       // 保留位
    } flags;
    uint8_t value;  // 整体访问接口
} __attribute__((packed)) PeriError_TypeDef;

typedef struct {
    volatile uint8_t rgb_update : 1;
    volatile uint8_t flash_config_update : 1;     // flash配置更新
    volatile uint8_t encoder_abmode_update : 1;   // 电机ab模式配置更新
    volatile uint8_t i2c_addr_config_update : 1;  // i2c地址配置更新
    volatile uint8_t jump_to_bootloader : 1;      // 跳转到bootloader
    volatile uint8_t rgb_busy : 1;
    volatile uint8_t reserved : 2;  // 预留位
} EventFlags_t;

extern volatile EventFlags_t g_events;
extern volatile PeriError_TypeDef peri_error;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
uint32_t millis(void);
uint32_t micros(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MCU_ENC_M2A_Pin       LL_GPIO_PIN_4
#define MCU_ENC_M2A_GPIO_Port GPIOA
#define MCU_ENC_M2A_EXTI_IRQn EXTI4_15_IRQn
#define MCU_ENC_M2B_Pin       LL_GPIO_PIN_5
#define MCU_ENC_M2B_GPIO_Port GPIOA
#define MCU_ENC_M1A_Pin       LL_GPIO_PIN_6
#define MCU_ENC_M1A_GPIO_Port GPIOA
#define MCU_ENC_M1A_EXTI_IRQn EXTI4_15_IRQn
#define MCU_ENC_M1B_Pin       LL_GPIO_PIN_7
#define MCU_ENC_M1B_GPIO_Port GPIOA
#define ADC_VM_Pin            LL_GPIO_PIN_0
#define ADC_VM_GPIO_Port      GPIOB
#define ADC_INA_Pin           LL_GPIO_PIN_1
#define ADC_INA_GPIO_Port     GPIOB
#define RGB_Pin               LL_GPIO_PIN_2
#define RGB_GPIO_Port         GPIOB
#define I2C_SCL_Pin           LL_GPIO_PIN_10
#define I2C_SCL_GPIO_Port     GPIOB
#define I2C_SDA_Pin           LL_GPIO_PIN_11
#define I2C_SDA_GPIO_Port     GPIOB
#define MCU_DIR_M2R_Pin       LL_GPIO_PIN_12
#define MCU_DIR_M2R_GPIO_Port GPIOB
#define MCU_DIR_M2F_Pin       LL_GPIO_PIN_13
#define MCU_DIR_M2F_GPIO_Port GPIOB
#define MCU_DIR_M1R_Pin       LL_GPIO_PIN_14
#define MCU_DIR_M1R_GPIO_Port GPIOB
#define MCU_DIR_M1F_Pin       LL_GPIO_PIN_15
#define MCU_DIR_M1F_GPIO_Port GPIOB
#define MCU_PWM_M2_Pin        LL_GPIO_PIN_8
#define MCU_PWM_M2_GPIO_Port  GPIOA
#define MCU_PWM_M1_Pin        LL_GPIO_PIN_9
#define MCU_PWM_M1_GPIO_Port  GPIOA
#define MCU_PWM_M4_Pin        LL_GPIO_PIN_10
#define MCU_PWM_M4_GPIO_Port  GPIOA
#define MCU_PWM_M3_Pin        LL_GPIO_PIN_11
#define MCU_PWM_M3_GPIO_Port  GPIOA
#define MCU_DIR_M3R_Pin       LL_GPIO_PIN_15
#define MCU_DIR_M3R_GPIO_Port GPIOA
#define MCU_DIR_M3F_Pin       LL_GPIO_PIN_3
#define MCU_DIR_M3F_GPIO_Port GPIOB
#define MCU_DIR_M4R_Pin       LL_GPIO_PIN_4
#define MCU_DIR_M4R_GPIO_Port GPIOB
#define MCU_DIR_M4F_Pin       LL_GPIO_PIN_5
#define MCU_DIR_M4F_GPIO_Port GPIOB
#define MCU_ENC_M3B_Pin       LL_GPIO_PIN_6
#define MCU_ENC_M3B_GPIO_Port GPIOB
#define MCU_ENC_M3A_Pin       LL_GPIO_PIN_7
#define MCU_ENC_M3A_GPIO_Port GPIOB
#define MCU_ENC_M3A_EXTI_IRQn EXTI4_15_IRQn
#define MCU_ENC_M4B_Pin       LL_GPIO_PIN_8
#define MCU_ENC_M4B_GPIO_Port GPIOB
#define MCU_ENC_M4A_Pin       LL_GPIO_PIN_9
#define MCU_ENC_M4A_GPIO_Port GPIOB
#define MCU_ENC_M4A_EXTI_IRQn EXTI4_15_IRQn

/* USER CODE BEGIN Private defines */
#define FIRMWARE_VERSION 0x01
#define I2C_ADDRESS      0x24
#define FLASH_DATA_SIZE  8

#define APPLICATION_ADDRESS ((uint32_t)0x08002800)
#define BOOTLOADER_ADDRESS  ((uint32_t)0x08000000)
#define BOOTLOADER_VER_ADDR ((uint32_t)0x08002800 - 1)

#define I2C_MIN_ADDR_VALUE 0x08
#define I2C_MAX_ADDR_VALUE 0x77

#define I2C_RX_MOTOR_THROTTLE_BASE 0x20
// #define I2C_RX_MOTOR_CONFIG_LIMIT           0x5C
#define I2C_RX_MOTOR_CONFIG_LIMIT 0x8C

#define I2C_RX_ENCODER_BASE   0x30
#define I2C_RX_ENCODER_0_ADDR 0x30
#define I2C_RX_ENCODER_1_ADDR 0x31
#define I2C_RX_ENCODER_2_ADDR 0x32
#define I2C_RX_ENCODER_3_ADDR 0x33
#define I2C_RX_ENCODER_LIMIT  0x33

#define I2C_RX_MOTOR_CONFIG_BASE  0x50
#define I2C_RX_MOTOR1_CONFIG_ADDR 0x50
#define I2C_RX_MOTOR2_CONFIG_ADDR 0x60
#define I2C_RX_MOTOR3_CONFIG_ADDR 0x70
#define I2C_RX_MOTOR4_CONFIG_ADDR 0x80
#define I2C_RX_MOTOR_CONFIG_COUNT 13

#define I2C_RX_SYSTEM_SETTINGS_BASE 0xD0
#define I2C_RX_SOFT_START_AND_STOP  0xD1
// #define I2C_RX_SYSTEM_SETTINGS_LIMIT        0xD2
#define I2C_RX_SYSTEM_SETTINGS_LIMIT 0xD1
#define I2C_TX_UID_VALUE_ADDR        0xE0
#define I2C_TX_UID_VALUE_MAX_ADDR    0xEB
#define I2C_RX_RGB_SETTINGS_BASE     0xC0
#define I2C_RX_RGB_SETTINGS_LIMIT    0xC4

#define I2C_TX_ERROR_BASE_ADDR         0xFB
#define I2C_RX_JUMP_TO_BOOTLOADER_ADDR 0xFD
#define I2C_RX_FIRMWARE_VERSION_ADDR   0xFE
#define I2C_RX_I2C_ADDR_CMD_ADDR       0xFF

#define I2C_TX_MOTOR_THROTTLE_ADDR 0x20

#define I2C_TX_ENCODER_VALUE_ADDR 0x30
#define I2C_TX_SPEED_VALUE_ADDR   0x40
#define I2C_TX_MOTOR_CONFIG_ADDR  0x50

#define I2C_TX_MOTOR1_CONFIG_ADDR 0x50
#define I2C_TX_MOTOR2_CONFIG_ADDR 0x60
#define I2C_TX_MOTOR3_CONFIG_ADDR 0x70
#define I2C_TX_MOTOR4_CONFIG_ADDR 0x80

#define I2C_TX_ADC_CURRENT_VALUE_ADDR 0x90
#define I2C_TX_ADC_CURRENT_MAX_ADDR   0x93
#define I2C_TX_ADC_INA_RAW_VALUE_ADDR 0xA0
#define I2C_TX_ADC_INA_RAW_MAX_ADDR   0xA1
#define I2C_TX_ADC_VM_VALUE_ADDR      0xB0
#define I2C_TX_MV_VM_VALUE_ADDR       0xB2
#define I2C_TX_ADC_VM_MAX_ADDR        0xB3

#define I2C_TX_SYSTEM_SETTINGS_ADDR 0xD0

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

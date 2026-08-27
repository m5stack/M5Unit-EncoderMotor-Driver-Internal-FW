/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    adc.h
 * @brief   This file contains all the function prototypes for
 *          the adc.c file
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
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN Private defines */
#define ADC_CHANNEL_NUMS  4
#define MAX_DELTA         100  // 允许相邻采样最大差值（根据ADC量程调整）
#define FILTER_WINDOW     10
#define MEDIAN_WINDOW     5
#define INA_REF_CAL_TIMES 100

typedef struct {
    uint32_t uiAdcValueBuf[ADC_CHANNEL_NUMS];
    uint32_t mirrorAdcValueBuf[ADC_CHANNEL_NUMS];
    float vout;
    float vout_diff;
    float i_in_value;
    float offset_val;  // 零点偏差
    uint32_t ref_voltage;
    uint32_t voltage_ina_mv;
    uint32_t voltage_vm_mv;
    int32_t current_ina_ma;
    uint8_t is_ina_ref_init;
    float ina_ref_voltage;
    int32_t ina_ref_voltage_mv;
    uint32_t ina_ref_cal_times;
} adc_data_t;

extern volatile adc_data_t adc_data;
/* USER CODE END Private defines */

void MX_ADC1_Init(void);

/* USER CODE BEGIN Prototypes */
uint16_t safe_adc_read(uint16_t new_val, uint16_t last_val);
uint16_t moving_average(uint16_t new_val, uint16_t* buf, uint8_t* idx);
uint16_t median_filter(uint16_t new_val, uint16_t* buffer, uint8_t* idx);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */

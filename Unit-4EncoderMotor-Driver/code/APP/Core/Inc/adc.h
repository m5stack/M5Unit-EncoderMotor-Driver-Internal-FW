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

extern ADC_HandleTypeDef hadc;

/* USER CODE BEGIN Private defines */
#define ADC_CHANNEL_NUMS      3U
#define ADC_AVERAGE_SAMPLES   256U
#define ADC_DMA_HALF_SCANS    (ADC_AVERAGE_SAMPLES / 2U)
#define ADC_DMA_HALF_LENGTH   (ADC_CHANNEL_NUMS * ADC_DMA_HALF_SCANS)
#define ADC_DMA_BUFFER_LENGTH (ADC_DMA_HALF_LENGTH * 2U)
#define MAX_DELTA             100  // 允许相邻采样最大差值（根据ADC量程调整）
#define FILTER_WINDOW         10
#define MEDIAN_WINDOW         5
#define INA_REF_CAL_TIMES     100
#define ADC_INA_CHANNEL_INDEX 1

typedef struct {
    uint32_t uiAdcValueBuf[ADC_CHANNEL_NUMS];           // 256-sample averages used by application/I2C
    uint16_t mirrorAdcValueBuf[ADC_DMA_BUFFER_LENGTH];  // Raw interleaved ADC DMA buffer
    uint16_t averagedAdcValueBuf[ADC_CHANNEL_NUMS];     // Completed 256-sample averages
    uint16_t adc_ina_raw;                               // Latest raw ADC count of ADC_INA
    uint8_t adc_average_ready;
    float vout;
    float vout_diff;
    float i_in_value;
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

void MX_ADC_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */

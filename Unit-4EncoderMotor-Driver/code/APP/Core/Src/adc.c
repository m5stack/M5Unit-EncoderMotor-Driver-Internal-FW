/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    adc.c
 * @brief   This file provides code for the configuration
 *          of the ADC instances.
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
#include "adc.h"

/* USER CODE BEGIN 0 */
#include <stdlib.h>
#include <string.h>
volatile adc_data_t adc_data = {0};

static uint32_t adc_dma_sum[ADC_CHANNEL_NUMS] = {0};
static uint16_t adc_dma_accumulated_scans     = 0;

static void accumulate_adc_dma_half(uint32_t start_index)
{
    for (uint32_t scan = 0; scan < ADC_DMA_HALF_SCANS; ++scan) {
        uint32_t scan_index = start_index + scan * ADC_CHANNEL_NUMS;
        for (uint32_t channel = 0; channel < ADC_CHANNEL_NUMS; ++channel) {
            adc_dma_sum[channel] += adc_data.mirrorAdcValueBuf[scan_index + channel];
        }
    }

    adc_data.adc_ina_raw =
        adc_data.mirrorAdcValueBuf[start_index + (ADC_DMA_HALF_SCANS - 1U) * ADC_CHANNEL_NUMS + ADC_INA_CHANNEL_INDEX];
    adc_dma_accumulated_scans += ADC_DMA_HALF_SCANS;

    if (adc_dma_accumulated_scans >= ADC_AVERAGE_SAMPLES) {
        for (uint32_t channel = 0; channel < ADC_CHANNEL_NUMS; ++channel) {
            adc_data.averagedAdcValueBuf[channel] = (uint16_t)(adc_dma_sum[channel] / ADC_AVERAGE_SAMPLES);
            adc_dma_sum[channel]                  = 0;
        }
        adc_dma_accumulated_scans  = 0;
        adc_data.adc_average_ready = 1;
    }
}
/* USER CODE END 0 */

ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_adc;

/* ADC init function */
void MX_ADC_Init(void)
{
    /* USER CODE BEGIN ADC_Init 0 */

    /* USER CODE END ADC_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC_Init 1 */

    /* USER CODE END ADC_Init 1 */

    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
     */
    hadc.Instance                   = ADC1;
    hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;
    hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc.Init.LowPowerAutoWait      = DISABLE;
    hadc.Init.LowPowerAutoPowerOff  = DISABLE;
    hadc.Init.ContinuousConvMode    = ENABLE;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.DMAContinuousRequests = ENABLE;
    hadc.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&hadc) != HAL_OK) {
        Error_Handler();
    }

    /** Configure for the selected ADC regular channel to be converted.
     */
    sConfig.Channel      = ADC_CHANNEL_8;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
        Error_Handler();
    }

    /** Configure for the selected ADC regular channel to be converted.
     */
    sConfig.Channel = ADC_CHANNEL_9;
    if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
        Error_Handler();
    }

    /** Configure for the selected ADC regular channel to be converted.
     */
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN ADC_Init 2 */

    /* USER CODE END ADC_Init 2 */
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *adcHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (adcHandle->Instance == ADC1) {
        /* USER CODE BEGIN ADC1_MspInit 0 */

        /* USER CODE END ADC1_MspInit 0 */
        /* ADC1 clock enable */
        __HAL_RCC_ADC1_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**ADC GPIO Configuration
        PB0     ------> ADC_IN8
        PB1     ------> ADC_IN9
        */
        GPIO_InitStruct.Pin  = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* ADC1 DMA Init */
        /* ADC Init */
        hdma_adc.Instance                 = DMA1_Channel1;
        hdma_adc.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma_adc.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_adc.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_adc.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
        hdma_adc.Init.Mode                = DMA_CIRCULAR;
        hdma_adc.Init.Priority            = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_adc) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(adcHandle, DMA_Handle, hdma_adc);

        /* USER CODE BEGIN ADC1_MspInit 1 */

        /* USER CODE END ADC1_MspInit 1 */
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *adcHandle)
{
    if (adcHandle->Instance == ADC1) {
        /* USER CODE BEGIN ADC1_MspDeInit 0 */

        /* USER CODE END ADC1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_ADC1_CLK_DISABLE();

        /**ADC GPIO Configuration
        PB0     ------> ADC_IN8
        PB1     ------> ADC_IN9
        */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0 | GPIO_PIN_1);

        /* ADC1 DMA DeInit */
        HAL_DMA_DeInit(adcHandle->DMA_Handle);
        /* USER CODE BEGIN ADC1_MspDeInit 1 */

        /* USER CODE END ADC1_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */

uint16_t safe_adc_read(uint16_t new_val, uint16_t last_val)
{
    if (abs(new_val - last_val) > MAX_DELTA && last_val) {
        return last_val;  // 丢弃异常值
    }
    return new_val;
}

// 滑动平均滤波函数
uint16_t moving_average(uint16_t new_val, uint16_t *buf, uint8_t *idx)
{
    buf[*idx] = new_val;
    *idx      = (*idx + 1) % FILTER_WINDOW;

    uint32_t sum = 0;
    for (int i = 0; i < FILTER_WINDOW; i++) {
        sum += buf[i];
    }
    return sum / FILTER_WINDOW;
}

// 复合滤波函数
uint16_t median_filter(uint16_t new_val, uint16_t *buffer, uint8_t *idx)
{
    static uint8_t is_buffer_filled = 0;
    const uint8_t window            = MEDIAN_WINDOW;
    uint8_t current_idx             = *idx;

    // 更新缓冲区
    buffer[current_idx] = new_val;
    current_idx         = (current_idx + 1) % window;
    *idx                = current_idx;

    // 检查缓冲区是否填满
    if (!is_buffer_filled && current_idx == 0) is_buffer_filled = 1;

    // 高效插入排序（利用缓冲区部分有序特性）
    uint16_t sorted_buf[MEDIAN_WINDOW];
    memcpy(sorted_buf, buffer, sizeof(sorted_buf));

    for (uint8_t i = 1; i < window; i++) {
        uint16_t key = sorted_buf[i];
        int8_t j     = i - 1;

        // 移动大于key的元素
        while (j >= 0 && sorted_buf[j] > key) {
            sorted_buf[j + 1] = sorted_buf[j];
            j--;
        }
        sorted_buf[j + 1] = key;
    }

    // 返回中值（奇数取中间，偶数取平均）
    if (window % 2 == 1) {
        return sorted_buf[window / 2];
    } else {
        return (sorted_buf[window / 2 - 1] + sorted_buf[window / 2]) / 2;
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc_handle)
{
    if (hadc_handle->Instance == ADC1) {
        accumulate_adc_dma_half(0U);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc_handle)
{
    if (hadc_handle->Instance == ADC1) {
        accumulate_adc_dma_half(ADC_DMA_HALF_LENGTH);
    }
}
/* USER CODE END 1 */

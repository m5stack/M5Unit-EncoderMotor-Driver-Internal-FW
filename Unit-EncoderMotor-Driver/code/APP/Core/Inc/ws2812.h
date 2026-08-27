/**
 ******************************************************************************
 * File Name          : gpio.h
 * Description        : This file contains all the functions prototypes for
 *                      the gpio
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ws2812_H__
#define __ws2812_H__
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define PIXEL_MAX          1
#define BIT_1              59
#define BIT_0              20
#define RGB_LED_TIMING_MAX 79

#define RGB_TIMEOUT 1

/* USER CODE END Private defines */
void sk6812_init(uint8_t num);
void neopixel_set_color(uint8_t num, uint32_t color, uint8_t brightness);
void ws2812_show(void);

/* USER CODE BEGIN Prototypes */
typedef struct {
    const uint16_t head[3];         // 先发送3个0等待DMA稳定
    uint16_t data[24 * PIXEL_MAX];  // 真正的数据
    const uint16_t tail;            // 发送最后1个0，保证DMA结束后，PWM输出低
} frame_buf;

typedef struct {
    union {
        struct {
            uint32_t rgb_data_b : 8;
            uint32_t rgb_data_g : 8;
            uint32_t rgb_data_r : 8;
            uint32_t rsvd : 8;
        } flags;
        uint32_t value;  // 整体访问接口
    } rgb_data;
    uint8_t brightness;
} rgb_data_t;

extern volatile rgb_data_t rgb_data;

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

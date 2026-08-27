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

/* USER CODE BEGIN Includes */
#include "main.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define TOTAL_RGB 1

#define gpio_low()  GPIOB->BRR = GPIO_PIN_2
#define gpio_high() GPIOB->BSRR = GPIO_PIN_2
#define delay_150ns() \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP")
#define delay_300ns() \
    delay_150ns();    \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP")
#define delay_600ns() \
    delay_300ns();    \
    delay_300ns()
#define delay_900ns() \
    delay_600ns();    \
    delay_300ns()

#define out_bit_low() \
    gpio_high();      \
    delay_300ns();    \
    gpio_low();       \
    delay_900ns();

#define out_bit_high() \
    gpio_high();       \
    delay_900ns();     \
    gpio_low();        \
    delay_300ns();

/* USER CODE END Private defines */
void restart(void);
void GPIO_init(void);
void sk6812_init(uint8_t num);
void neopixel_set_color(uint8_t num, uint32_t color);
void neopixel_set_all_color(uint32_t *color);
void neopixel_show(void);
void color_set_single(uint32_t color);
void neopixel_set_single_color(uint8_t num, uint8_t color, uint8_t shiftnum);

//// WS2812配置
// #define WS2812_PIN          GPIO_PIN_2
// #define WS2812_PORT         GPIOB
// #define WS2812_GPIO_CLK     __HAL_RCC_GPIOB_CLK_ENABLE

// // 颜色定义 (RGB格式输入)
// #define COLOR_OFF           0x000000
// #define COLOR_RED           0xFF0000
// #define COLOR_GREEN         0x00FF00
// #define COLOR_BLUE          0x0000FF
// #define COLOR_WHITE         0xFFFFFF
// #define COLOR_YELLOW        0xFFFF00
// #define COLOR_CYAN          0x00FFFF
// #define COLOR_MAGENTA       0xFF00FF
//
// // 时序参数 (基于48MHz系统时钟，每个周期约20.83ns)
// // T0H: 220ns-380ns，取300ns -> 300ns/20.83ns ≈ 14个周期
// #define T0H_CYCLES          14
//
// // T0L: 580ns-1μs，取800ns -> 800ns/20.83ns ≈ 38个周期
// #define T0L_CYCLES          38
//
// // T1H: 580ns-1μs，取800ns -> 800ns/20.83ns ≈ 38个周期
// #define T1H_CYCLES          38
//
// // T1L: 220ns-420ns，取300ns -> 300ns/20.83ns ≈ 14个周期
// #define T1L_CYCLES          14
//
//
//// 精确延时宏
// # define DELAY_CYCLES(cycles) do { \
// volatile uint32_t delay_count = cycles; \
// while(delay_count--) { \
//	 __NOP(); \
// } \
//} while(0)
//
//// GPIO快速操作宏
// #define WS2812_HIGH()       (WS2812_PORT->BSRR = WS2812_PIN)
// #define WS2812_LOW()        (WS2812_PORT->BSRR = (WS2812_PIN << 16))
//
//

#define PIXEL_MAX          1
#define BIT_1              59
#define BIT_0              20
#define RGB_LED_TIMING_MAX 79

#define RGB_TIMEOUT 1

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

void WS2812_Init(void);
void WS2812_SetColor(uint32_t color);
void WS2812_Off(void);
uint32_t WS2812_RGB(uint8_t r, uint8_t g, uint8_t b);
uint32_t WS2812_Brightness(uint32_t color, uint8_t brightness);

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

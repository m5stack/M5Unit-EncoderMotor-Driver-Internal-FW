#include "stm32f0xx_hal.h"
#include "ws2812.h"
#include <stdlib.h>

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
 * Analog
 * Input
 * Output
 * EVENT_OUT
 * EXTI
 */

uint8_t led_num;
uint32_t *color_buf = NULL;
uint8_t _brightness = 0;
void color_set_single(uint32_t color);
uint8_t rled[TOTAL_RGB] = {0};
uint8_t gled[TOTAL_RGB] = {0};
uint8_t bled[TOTAL_RGB] = {0};

void restart(void)
{
    for (uint8_t i = 0; i < 113; i++) {
        delay_600ns();
    }
}

void GPIO_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = GPIO_PIN_6;
    GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull             = GPIO_NOPULL;
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void sk6812_init(uint8_t num)
{
    /**/
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin                 = RGB_Pin;
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed               = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType          = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull                = LL_GPIO_PULL_NO;
    LL_GPIO_Init(RGB_GPIO_Port, &GPIO_InitStruct);

    color_buf = (uint32_t *)calloc(num, sizeof(uint32_t));
    led_num   = num;
}

uint8_t min(uint8_t num1, uint8_t num2)
{
    return (num1 < num2) ? num1 : num2;
}

uint8_t max(uint8_t num1, uint8_t num2)
{
    return (num1 > num2) ? num1 : num2;
}

/**
 * @brief 应用亮度到颜色值
 * @param color: 原始颜色值
 * @param brightness: 亮度 0-100
 * @return: 调整亮度后的颜色值
 */
uint8_t apply_brightness(uint8_t color, uint8_t brightness)
{
    return (uint8_t)(color * (float)brightness / 255.0f);
}

/**
 * @brief 设置单个LED的颜色和亮度
 * @param num: LED编号
 * @param color: 颜色值 (RGB格式)
 * @param brightness: 亮度 0-100
 */
void neopixel_set_color_brightness(uint8_t num, uint32_t color, uint8_t brightness)
{
    if (num >= led_num || brightness > 255) return;

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    // 应用指定亮度
    r = apply_brightness(r, brightness);
    g = apply_brightness(g, brightness);
    b = apply_brightness(b, brightness);

    // 存储到数组
    rled[num] = r;
    gled[num] = g;
    bled[num] = b;

    // WS2812使用GRB格式
    color_buf[num] = (g << 16) | (r << 8) | b;
    _brightness    = brightness;  // update brightness
}

void neopixel_set_color(uint8_t num, uint32_t color)
{
    uint8_t bled   = (color >> 16) & 0xff;
    uint8_t gled   = (color >> 8) & 0xff;
    uint8_t rled   = (color) & 0xff;
    color_buf[num] = gled << 16 | rled << 8 | bled;
}

void neopixel_set_single_color(uint8_t num, uint8_t color, uint8_t shiftnum)
{
    if (shiftnum == 0)
        rled[num] = apply_brightness(color, _brightness);
    else if (shiftnum == 1)
        gled[num] = apply_brightness(color, _brightness);
    else if (shiftnum == 2)
        bled[num] = apply_brightness(color, _brightness);
    color_buf[num] = gled[num] << 16 | rled[num] << 8 | bled[num];
}

void neopixel_set_all_color(uint32_t *color)
{
    for (uint8_t i = 0; i < led_num; i++) {
        uint8_t bled = (color[i] >> 16) & 0xff;
        uint8_t gled = (color[i] >> 8) & 0xff;
        uint8_t rled = color[i] & 0xff;
        color_buf[i] = gled << 16 | rled << 8 | bled;
    }
}

void neopixel_show(void)
{
    __disable_irq();
    for (uint8_t i = 0; i < led_num; i++) {
        color_set_single(color_buf[i]);
    }
    restart();
    __enable_irq();
}

void color_set_single(uint32_t color)
{
    for (uint8_t i = 0; i < 24; i++) {
        if (color & (1 << (23 - i))) {
            out_bit_high();
        } else {
            out_bit_low();
        }
    }
}

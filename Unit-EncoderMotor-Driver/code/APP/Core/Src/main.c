/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mymotorencoder.h"
#include "mymotorbdc.h"
#include "ws2812.h"
#include "pt_task.h"
#include "sys_utils.h"
#include "flash.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_CH_INA        0  // 根据你的通道顺序修改
#define ADC_CH_REF        3  // 根据你的通道顺序修改
#define ADC_CALIB_SAMPLES 100

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile PeriError_TypeDef peri_error = {0};
volatile EventFlags_t g_events        = {0};
static uint8_t i2c_handle_tx_buf[128];
volatile uint8_t fm_version = FIRMWARE_VERSION;

// Device Unique identifier
__IO uint8_t g_uid[12] = {0};  // Unique identifier
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t millis(void)
{
    return HAL_GetTick();
}

__STATIC_INLINE uint32_t GXT_SYSTICK_IsActiveCounterFlag(void)
{
    return ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == (SysTick_CTRL_COUNTFLAG_Msk));
}

static uint32_t getCurrentMicros(void)
{
    /* Ensure COUNTFLAG is reset by reading SysTick control and status register */
    GXT_SYSTICK_IsActiveCounterFlag();
    uint32_t m         = HAL_GetTick();
    const uint32_t tms = SysTick->LOAD + 1;
    __IO uint32_t u    = tms - SysTick->VAL;
    if (GXT_SYSTICK_IsActiveCounterFlag()) {
        m = HAL_GetTick();
        u = tms - SysTick->VAL;
    }
    return (m * 1000 + (u * 1000) / tms);
}

// 获取系统时间，单位us
uint32_t micros(void)
{
    return getCurrentMicros();
}

static void i2c_send_bytes_from_offset(uint8_t *data, uint8_t data_len, uint8_t offset)
{
    if (offset >= data_len) {
        i2c2_set_send_data(NULL, 0);
        return;
    }

    i2c2_set_send_data(&data[offset], data_len - offset);
}

static void i2c_send_u8(uint8_t data)
{
    i2c_handle_tx_buf[0] = data;
    i2c2_set_send_data(i2c_handle_tx_buf, 1);
}

static void i2c_send_u16_from_offset(uint16_t data, uint8_t offset)
{
    if (offset >= 2) {
        i2c2_set_send_data(NULL, 0);
        return;
    }

    memcpy(i2c_handle_tx_buf, &data, 2);
    i2c2_set_send_data(&i2c_handle_tx_buf[offset], 2 - offset);
}

static void i2c_send_u32_from_offset(uint32_t data, uint8_t offset)
{
    if (offset >= 4) {
        i2c2_set_send_data(NULL, 0);
        return;
    }

    memcpy(i2c_handle_tx_buf, &data, 4);
    i2c2_set_send_data(&i2c_handle_tx_buf[offset], 4 - offset);
}

static void i2c_send_float_from_offset(float data, uint8_t offset)
{
    if (offset >= 4) {
        i2c2_set_send_data(NULL, 0);
        return;
    }

    memcpy(i2c_handle_tx_buf, &data, 4);
    i2c2_set_send_data(&i2c_handle_tx_buf[offset], 4 - offset);
}

static uint8_t is_valid_motor_config_value(uint8_t offset, uint8_t value)
{
    switch (offset) {
        case MOTOR_CONFIG_MOTOR_MODE:
            return value <= 0x02;
        case MOTOR_CONFIG_MOTOR_POS_MAX_SPEED:
            return value != 0x80;
        case MOTOR_CONFIG_MOTOR_SPEED_POINT:
            return value != 0x80;
        default:
            return 1;
    }
}

static uint8_t handle_i2c_write(uint8_t reg, uint8_t value)
{
    if (reg == I2C_RX_MOTOR_THROTTLE_BASE) {
        motor_data.motor_throttle[0] = value;
    } else if ((reg >= I2C_RX_ENCODER_BASE) && (reg <= I2C_RX_ENCODER_LIMIT)) {
        uint8_t offset                                                  = reg - I2C_RX_ENCODER_BASE;
        ((volatile uint8_t *)&encoder_raw[0].encoder_raw.value)[offset] = value;
    } else if ((reg >= I2C_RX_MOTOR_CONFIG_BASE) && (reg < I2C_RX_MOTOR_CONFIG_BASE + I2C_RX_MOTOR_CONFIG_COUNT)) {
        uint8_t offset = reg - I2C_RX_MOTOR_CONFIG_BASE;
        if (is_valid_motor_config_value(offset, value)) {
            motor_data.config[MOTOR_M1][offset] = value;
        }
    } else if ((reg >= I2C_RX_SYSTEM_SETTINGS_BASE) && (reg <= I2C_RX_SYSTEM_SETTINGS_LIMIT)) {
        switch (reg) {
            case I2C_RX_SYSTEM_SETTINGS_BASE: {
                uint8_t encoder_ab_mode = value ? 1 : 0;
                if (motor_data.encoder_ab_mode != encoder_ab_mode) {
                    motor_data.encoder_ab_mode     = encoder_ab_mode;
                    g_events.flash_config_update   = 1;
                    g_events.encoder_abmode_update = 1;  // 设置电机ab模式配置更新标志
                }
                break;
            }
            case I2C_RX_SYSTEM_SETTINGS_BASE + 1:
                if (motor_data.soft_start_stop_switch.value != value) {
                    motor_data.soft_start_stop_switch.value = value;
                    g_events.flash_config_update            = 1;
                }
                break;
            case I2C_RX_SYSTEM_SETTINGS_BASE + 2:
                motor_data.motor_sleep = value;
                if (motor_data.motor_sleep) {
                    LL_GPIO_SetOutputPin(DRV_NSLEEP_GPIO_Port, DRV_NSLEEP_Pin);
                } else {
                    LL_GPIO_ResetOutputPin(DRV_NSLEEP_GPIO_Port, DRV_NSLEEP_Pin);
                }
                break;
            default:
                break;
        }
    } else if ((reg >= I2C_RX_RGB_SETTINGS_BASE) && (reg <= I2C_RX_RGB_SETTINGS_LIMIT)) {
        switch (reg) {
            case I2C_RX_RGB_SETTINGS_BASE:
                rgb_data.rgb_data.flags.rgb_data_b = value;
                return 1;
            case I2C_RX_RGB_SETTINGS_BASE + 1:
                rgb_data.rgb_data.flags.rgb_data_g = value;
                return 1;
            case I2C_RX_RGB_SETTINGS_BASE + 2:
                rgb_data.rgb_data.flags.rgb_data_r = value;
                return 1;
            case I2C_RX_RGB_SETTINGS_BASE + 4:
                rgb_data.brightness = value;
                return 1;
            default:
                break;
        }
    }

    return 0;
}

static void handle_i2c_read(uint8_t reg)
{
    if (reg == I2C_TX_MOTOR_THROTTLE_ADDR) {
        i2c_send_u8(motor_data.motor_throttle[0]);
    } else if ((reg >= I2C_TX_ENCODER_VALUE_ADDR) && (reg <= I2C_RX_ENCODER_LIMIT)) {
        i2c_send_u32_from_offset((uint32_t)encoder_raw[0].encoder_raw.value, reg - I2C_TX_ENCODER_VALUE_ADDR);
    } else if (reg == I2C_TX_SPEED_VALUE_ADDR) {
        i2c_send_u8(motor_data.speed[0]);
    } else if ((reg >= I2C_TX_MOTOR_CONFIG_ADDR) && (reg < I2C_TX_MOTOR_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
        i2c_send_bytes_from_offset((uint8_t *)&motor_data.config[MOTOR_M1][0], I2C_RX_MOTOR_CONFIG_COUNT,
                                   reg - I2C_TX_MOTOR_CONFIG_ADDR);
    } else if ((reg >= I2C_TX_ADC_CURRENT_VALUE_ADDR) && (reg < I2C_TX_ADC_CURRENT_VALUE_ADDR + 4)) {
        i2c_send_float_from_offset(adc_data.i_in_value, reg - I2C_TX_ADC_CURRENT_VALUE_ADDR);
    } else if ((reg >= I2C_TX_ADC_VM_VALUE_ADDR) && (reg < I2C_TX_ADC_VM_VALUE_ADDR + 2)) {
        i2c_send_u16_from_offset((uint16_t)adc_data.uiAdcValueBuf[1], reg - I2C_TX_ADC_VM_VALUE_ADDR);
    } else if ((reg >= I2C_TX_MV_VM_VALUE_ADDR) && (reg < I2C_TX_MV_VM_VALUE_ADDR + 2)) {
        i2c_send_u16_from_offset((uint16_t)adc_data.voltage_vm_mv, reg - I2C_TX_MV_VM_VALUE_ADDR);
    } else if ((reg >= I2C_RX_SYSTEM_SETTINGS_BASE) && (reg <= I2C_RX_SYSTEM_SETTINGS_LIMIT)) {
        i2c_handle_tx_buf[0] = motor_data.encoder_ab_mode;
        i2c_handle_tx_buf[1] = motor_data.soft_start_stop_switch.value;
        i2c_handle_tx_buf[2] = motor_data.motor_sleep;
        i2c2_set_send_data(&i2c_handle_tx_buf[reg - I2C_RX_SYSTEM_SETTINGS_BASE],
                           I2C_RX_SYSTEM_SETTINGS_LIMIT - reg + 1);
    } else if ((reg >= I2C_RX_RGB_SETTINGS_BASE) && (reg <= I2C_RX_RGB_SETTINGS_LIMIT)) {
        memcpy(&i2c_handle_tx_buf[0], (uint8_t *)&rgb_data.rgb_data.value, 4);
        i2c_handle_tx_buf[4] = rgb_data.brightness;
        i2c2_set_send_data(&i2c_handle_tx_buf[reg - I2C_RX_RGB_SETTINGS_BASE], I2C_RX_RGB_SETTINGS_LIMIT - reg + 1);
    } else if ((reg >= I2C_TX_UID_BASE) && (reg <= I2C_TX_UID_LIMIT)) {
        for (uint8_t i = 0; i <= (I2C_TX_UID_LIMIT - I2C_TX_UID_BASE); i++) {
            i2c_handle_tx_buf[i] = g_uid[i];
        }
        i2c2_set_send_data(&i2c_handle_tx_buf[reg - I2C_TX_UID_BASE], I2C_TX_UID_LIMIT - reg + 1);
    } else if (reg == I2C_TX_DEVICE_STATUS_ADDR) {
        i2c_send_u8(1);
    } else if (reg == I2C_RX_JUMP_TO_BOOTLOADER_ADDR) {
        i2c_send_u8(0);
    } else if (reg == I2C_TX_FIRMWARE_VERSION_ADDR) {
        i2c_send_u8(fm_version);
    } else if (reg == I2C_RX_I2C_ADDR_CMD_ADDR) {
        i2c_send_u8(i2c_address[0]);
    } else {
        i2c2_set_send_data(NULL, 0);
    }
}

void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len)
{
    if (len > 1) {
        uint8_t rgb_changed = 0;

        for (uint16_t i = 1; i < len; i++) {
            uint16_t reg = (uint16_t)rx_data[0] + i - 1;
            if (reg > 0xFF) {
                break;
            }

            rgb_changed |= handle_i2c_write((uint8_t)reg, rx_data[i]);
        }

        if (rgb_changed) {
            enqueue_config_request(rgb_data);
            g_events.rgb_update = 1;
        }

        if (rx_data[0] == I2C_RX_I2C_ADDR_CMD_ADDR) {
            if (len == 2) {
                if ((rx_data[1] <= I2C_MAX_ADDR_VALUE) && (rx_data[1] >= I2C_MIN_ADDR_VALUE)) {
                    if (i2c_address[0] != rx_data[1]) {
                        i2c_address[0]                  = rx_data[1];
                        g_events.i2c_addr_config_update = 1;
                        g_events.flash_config_update    = 1;
                    }
                }
            }
        } else if (rx_data[0] == I2C_RX_JUMP_TO_BOOTLOADER_ADDR) {
            if (rx_data[1] == 1) {
                g_events.jump_to_bootloader = 1;
            }
        }
    } else if (len == 1) {
        handle_i2c_read(rx_data[0]);
    }
}

// ADC零点漂移校准
void adc_zero_calibration(void)
{
    // 使用独立的中值和均值缓冲区
    static uint16_t median_buf[ADC_CHANNEL_NUMS][MEDIAN_WINDOW] = {0};
    static uint8_t median_idx[ADC_CHANNEL_NUMS]                 = {0};
    static uint16_t avg_buf[ADC_CHANNEL_NUMS][FILTER_WINDOW]    = {0};
    static uint8_t avg_idx[ADC_CHANNEL_NUMS]                    = {0};
    static uint16_t last_adc_val[ADC_CHANNEL_NUMS]              = {0};
    static int32_t offset_sum                                   = 0;

    LL_GPIO_ResetOutputPin(DRV_NSLEEP_GPIO_Port, DRV_NSLEEP_Pin);

    for (int n = 0; n < ADC_CALIB_SAMPLES; n++) {
        // ------ 读取 INA ------
        uint16_t raw_ina         = safe_adc_read(adc_data.uiAdcValueBuf[ADC_CH_INA], last_adc_val[ADC_CH_INA]);
        last_adc_val[ADC_CH_INA] = raw_ina;
        uint16_t median_ina      = median_filter(raw_ina, median_buf[ADC_CH_INA], &median_idx[ADC_CH_INA]);
        uint16_t avg_ina         = moving_average(median_ina, avg_buf[ADC_CH_INA], &avg_idx[ADC_CH_INA]);

        // ------ 读取 REF ------
        uint16_t raw_ref         = safe_adc_read(adc_data.uiAdcValueBuf[ADC_CH_REF], last_adc_val[ADC_CH_REF]);
        last_adc_val[ADC_CH_REF] = raw_ref;
        uint16_t median_ref      = median_filter(raw_ref, median_buf[ADC_CH_REF], &median_idx[ADC_CH_REF]);
        uint16_t avg_ref         = moving_average(median_ref, avg_buf[ADC_CH_REF], &avg_idx[ADC_CH_REF]);

        // 差值累加
        offset_sum += (int32_t)avg_ina - (int32_t)avg_ref;

        // 小延时，不然DMA的ADC数据还可能没更新
        HAL_Delay(5);
    }
    adc_data.offset_val = (float)(offset_sum / ADC_CALIB_SAMPLES);
    LL_GPIO_SetOutputPin(DRV_NSLEEP_GPIO_Port, DRV_NSLEEP_Pin);
}

void read_uid(void)
{
    uint32_t uid_words[3] = {
        HAL_GetUIDw0(),
        HAL_GetUIDw1(),
        HAL_GetUIDw2(),
    };
    uint8_t *uid_bytes = (uint8_t *)uid_words;

    for (uint8_t i = 0; i < sizeof(g_uid); i++) {
        g_uid[i] = uid_bytes[i];
    }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    read_uid();
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_TIM3_Init();
    MX_I2C2_Init();
    MX_TIM16_Init();
    MX_IWDG_Init();
    /* USER CODE BEGIN 2 */
    HAL_IWDG_Refresh(&hiwdg);
    init_flash_data();
    user_gpio_init();
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
        peri_error.flags.adc_setup = 1;
    }
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_data.uiAdcValueBuf[0], ADC_CHANNEL_NUMS) != HAL_OK) {
        peri_error.flags.adc_setup = 1;
    }
    HAL_IWDG_Refresh(&hiwdg);
    adc_zero_calibration();
    HAL_IWDG_Refresh(&hiwdg);
    sk6812_init(PIXEL_MAX);
    neopixel_set_color(0, 0x000000, 50);
    ws2812_show();
    InitMotorPwm();
    InitMotorEncoder();
    init_motor();
    init_pt_task();
    user_i2c_init();
    i2c2_it_enable();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        main_scheduler(&main_pt);
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState            = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN            = 8;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
        HAL_NVIC_SystemReset();
    }
    /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

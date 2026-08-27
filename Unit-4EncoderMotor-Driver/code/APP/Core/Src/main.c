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
#include "../myfiles/motorencoder.h"
#include "../myfiles/motorbdc.h"
#include "../myfiles/ws2812.h"
#include "../myfiles/pt_task.h"
#include "../myfiles/sys_utils.h"
#include "../myfiles/flash.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile PeriError_TypeDef peri_error = {0};
volatile EventFlags_t g_events        = {0};
static uint8_t i2c_handle_tx_buf[128];
volatile uint8_t *p_bootloader_version = (volatile uint8_t *)BOOTLOADER_VER_ADDR;
volatile uint8_t fm_version            = FIRMWARE_VERSION;

volatile rgb_data_t rgb_data = {0};

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

static void Prepare_I2C_Read_Response(uint8_t reg)
{
    i2c2_set_send_data(NULL, 0);

    if ((reg >= I2C_RX_MOTOR_THROTTLE_BASE) && (reg <= I2C_RX_MOTOR_CONFIG_LIMIT)) {
        if ((reg >= I2C_RX_MOTOR_THROTTLE_BASE) && (reg <= (I2C_RX_MOTOR_THROTTLE_BASE + 3))) {
            uint8_t motor_id     = reg - I2C_RX_MOTOR_THROTTLE_BASE;
            i2c_handle_tx_buf[0] = motor_data.motor_throttle[motor_id];
            i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], 1);
        } else if ((reg >= I2C_RX_ENCODER_BASE) && (reg <= (I2C_RX_ENCODER_BASE + 15))) {
            uint8_t encoder_offset = reg - I2C_RX_ENCODER_BASE;
            uint8_t motor_id       = encoder_offset / 4;
            uint8_t byte_index     = encoder_offset % 4;

            if (motor_id < 4) {
                uint32_t raw_value = encoder_raw[motor_id].encoder_raw.value;
                uint8_t encoder_bytes[4];
                encoder_bytes[3] = (raw_value >> 24) & 0xFF;
                encoder_bytes[2] = (raw_value >> 16) & 0xFF;
                encoder_bytes[1] = (raw_value >> 8) & 0xFF;
                encoder_bytes[0] = raw_value & 0xFF;

                uint8_t remaining_bytes = 4 - byte_index;
                memcpy(&i2c_handle_tx_buf[0], &encoder_bytes[byte_index], remaining_bytes);
                i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], remaining_bytes);
            }
        } else if ((reg >= I2C_TX_SPEED_VALUE_ADDR) && (reg <= (I2C_TX_SPEED_VALUE_ADDR + 3))) {
            uint8_t motor_id = reg - I2C_TX_SPEED_VALUE_ADDR;
            if (motor_id < 4) {
                i2c_handle_tx_buf[0] = motor_data.speed[motor_id];
                i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], 1);
            }
        } else if ((reg >= I2C_RX_MOTOR_CONFIG_BASE) && (reg <= I2C_RX_MOTOR_CONFIG_LIMIT)) {
            uint8_t config_index = 0;
            uint8_t motor_id     = 4;

            if ((reg >= I2C_RX_MOTOR1_CONFIG_ADDR) && (reg < I2C_RX_MOTOR1_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
                config_index = reg - I2C_RX_MOTOR1_CONFIG_ADDR;
                motor_id     = 0;
            } else if ((reg >= I2C_RX_MOTOR2_CONFIG_ADDR) &&
                       (reg < I2C_RX_MOTOR2_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
                config_index = reg - I2C_RX_MOTOR2_CONFIG_ADDR;
                motor_id     = 1;
            } else if ((reg >= I2C_RX_MOTOR3_CONFIG_ADDR) &&
                       (reg < I2C_RX_MOTOR3_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
                config_index = reg - I2C_RX_MOTOR3_CONFIG_ADDR;
                motor_id     = 2;
            } else if ((reg >= I2C_RX_MOTOR4_CONFIG_ADDR) &&
                       (reg < I2C_RX_MOTOR4_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
                config_index = reg - I2C_RX_MOTOR4_CONFIG_ADDR;
                motor_id     = 3;
            }

            if ((motor_id < 4) && (config_index < I2C_RX_MOTOR_CONFIG_COUNT)) {
                uint8_t remaining_configs = I2C_RX_MOTOR_CONFIG_COUNT - config_index;
                memcpy(&i2c_handle_tx_buf[0], &motor_data.config[motor_id][config_index], remaining_configs);
                i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], remaining_configs);
            }
        }
    } else if ((reg >= I2C_TX_ADC_CURRENT_VALUE_ADDR) && (reg <= I2C_TX_ADC_CURRENT_MAX_ADDR)) {
        memcpy(&i2c_handle_tx_buf[0], (uint8_t *)&adc_data.i_in_value, 4);
        i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[reg - I2C_TX_ADC_CURRENT_VALUE_ADDR],
                           I2C_TX_ADC_CURRENT_MAX_ADDR - reg + 1);
    } else if ((reg >= I2C_TX_ADC_INA_RAW_VALUE_ADDR) && (reg <= I2C_TX_ADC_INA_RAW_MAX_ADDR)) {
        uint16_t adc_ina_raw = adc_data.adc_ina_raw;
        i2c_handle_tx_buf[0] = (uint8_t)(adc_ina_raw & 0xFF);
        i2c_handle_tx_buf[1] = (uint8_t)((adc_ina_raw >> 8) & 0xFF);
        i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[reg - I2C_TX_ADC_INA_RAW_VALUE_ADDR],
                           I2C_TX_ADC_INA_RAW_MAX_ADDR - reg + 1);
    } else if ((reg >= I2C_TX_ADC_VM_VALUE_ADDR) && (reg <= I2C_TX_ADC_VM_MAX_ADDR)) {
        memcpy(&i2c_handle_tx_buf[0], (uint8_t *)&adc_data.uiAdcValueBuf[1], 2);
        memcpy(&i2c_handle_tx_buf[I2C_TX_MV_VM_VALUE_ADDR - I2C_TX_ADC_VM_VALUE_ADDR],
               (uint8_t *)&adc_data.voltage_vm_mv, 2);
        i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[reg - I2C_TX_ADC_VM_VALUE_ADDR],
                           I2C_TX_ADC_VM_MAX_ADDR - reg + 1);
    } else if (reg == I2C_RX_SYSTEM_SETTINGS_BASE) {
        i2c_handle_tx_buf[0] = motor_data.encoder_ab_mode;
        i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], 1);
    } else if (reg == I2C_RX_SOFT_START_AND_STOP) {
        i2c_handle_tx_buf[0] = motor_data.soft_start_stop_switch.value;
        i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], 1);
    } else if ((reg >= I2C_TX_UID_VALUE_ADDR) && (reg <= I2C_TX_UID_VALUE_MAX_ADDR)) {
        uint8_t uid_offset = reg - I2C_TX_UID_VALUE_ADDR;
        if (uid_offset < sizeof(g_uid)) {
            uint8_t remaining_uid_bytes = sizeof(g_uid) - uid_offset;
            memcpy(&i2c_handle_tx_buf[0], (uint8_t *)&g_uid[uid_offset], remaining_uid_bytes);
            i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], remaining_uid_bytes);
        }
    } else if ((reg >= I2C_RX_RGB_SETTINGS_BASE) && (reg <= I2C_RX_RGB_SETTINGS_LIMIT)) {
        if (reg == I2C_RX_RGB_SETTINGS_BASE) {
            memcpy(&i2c_handle_tx_buf[0], (uint8_t *)&rgb_data.rgb_data.value, 4);
            i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], 4);
        } else if (reg == I2C_RX_RGB_SETTINGS_BASE + 4) {
            i2c_handle_tx_buf[0] = rgb_data.brightness;
            i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[0], 1);
        }
    } else if ((reg >= I2C_TX_ERROR_BASE_ADDR) && (reg <= I2C_RX_I2C_ADDR_CMD_ADDR)) {
        i2c_handle_tx_buf[0] = peri_error.value;
        i2c_handle_tx_buf[1] = *p_bootloader_version;
        i2c_handle_tx_buf[2] = 0;
        i2c_handle_tx_buf[3] = fm_version;
        i2c_handle_tx_buf[4] = i2c_address[0];
        i2c2_set_send_data((uint8_t *)&i2c_handle_tx_buf[reg - I2C_TX_ERROR_BASE_ADDR],
                           I2C_RX_I2C_ADDR_CMD_ADDR - reg + 1);
    }
}

/** four */
void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len)
{
    if (len == 1) {
        Prepare_I2C_Read_Response(rx_data[0]);
        return;
    }

    uint8_t rx_buf[128]  = {0};
    uint8_t rx_mark[128] = {0};

    // 主机写数据
    if (len > 1) {
        if ((rx_data[0] >= I2C_RX_MOTOR_THROTTLE_BASE) && (rx_data[0] <= I2C_RX_MOTOR_CONFIG_LIMIT)) {  // Motor Control
            for (int i = 0; i < len - 1; i++) {
                uint16_t rx_index = rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE + i;  // Get Control Register Index
                if (rx_index < sizeof(rx_buf)) {
                    rx_buf[rx_index]  = rx_data[1 + i];
                    rx_mark[rx_index] = 1;
                }
            }

            for (int i = 0; i < len - 1; i++) {
                uint16_t reg = (uint16_t)rx_data[0] + i;
                if ((reg >= I2C_RX_MOTOR_THROTTLE_BASE) && (reg <= (I2C_RX_MOTOR_THROTTLE_BASE + 3))) {
                    motor_data.motor_throttle[reg - I2C_RX_MOTOR_THROTTLE_BASE] = rx_data[1 + i];
                }
            }

            // 处理电机周期控制 (0x20-0x23)
            if ((rx_data[0] >= I2C_RX_MOTOR_THROTTLE_BASE) && (rx_data[0] <= (I2C_RX_MOTOR_THROTTLE_BASE + 3))) {
                uint8_t motor_id = rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE;   // 0,1,2,3
                if (rx_mark[rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE] == 1) {  // Get Specific motor id and data
                    motor_data.motor_throttle[motor_id] = rx_buf[rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE];
                }
            }

            // 处理编码器数据 (0x30-0x3F)
            if ((rx_data[0] >= I2C_RX_ENCODER_BASE) && (rx_data[0] <= (I2C_RX_ENCODER_BASE + 0x0F))) {
                uint8_t encoder_offset = rx_data[0] - I2C_RX_ENCODER_BASE;
                uint8_t motor_id       = encoder_offset / 4;  // 每个电机占用4个字节
                uint8_t byte_index     = encoder_offset % 4;  // 字节索引 (0,1,2,3)

                if (motor_id < 4) {  // 确保电机ID有效
                    for (int i = 0; i < 4; i++) {
                        if (((byte_index + i) < 4) && (rx_mark[rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE + i] == 1)) {
                            switch (byte_index + i) {
                                case 0:  // LSB
                                    encoder_raw[motor_id].encoder_raw.flags.encoder_raw_0 =
                                        rx_buf[rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE + i];
                                    break;
                                case 1:
                                    encoder_raw[motor_id].encoder_raw.flags.encoder_raw_1 =
                                        rx_buf[rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE + i];
                                    break;
                                case 2:
                                    encoder_raw[motor_id].encoder_raw.flags.encoder_raw_2 =
                                        rx_buf[rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE + i];
                                    break;
                                case 3:  // MSB
                                    encoder_raw[motor_id].encoder_raw.flags.encoder_raw_3 =
                                        rx_buf[rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE + i];
                                    break;
                            }
                        }
                    }
                }
            }

            // 处理电机配置数据 (0x50-0x8C)
            if ((rx_data[0] >= I2C_RX_MOTOR_CONFIG_BASE) && (rx_data[0] <= I2C_RX_MOTOR_CONFIG_LIMIT)) {
                uint8_t config_index = 0;
                uint8_t motor_id     = 4;

                // 配置电机0 0x50_0x5C
                if ((rx_data[0] >= I2C_RX_MOTOR1_CONFIG_ADDR) &&
                    (rx_data[0] < I2C_RX_MOTOR1_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
                    config_index = rx_data[0] - I2C_RX_MOTOR1_CONFIG_ADDR;
                    motor_id     = 0;
                }
                // 配置电机1 0x60_0x6C
                else if ((rx_data[0] >= I2C_RX_MOTOR2_CONFIG_ADDR) &&
                         (rx_data[0] < I2C_RX_MOTOR2_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
                    config_index = rx_data[0] - I2C_RX_MOTOR2_CONFIG_ADDR;
                    motor_id     = 1;
                }
                // 配置电机2 0x70_0x7C
                else if ((rx_data[0] >= I2C_RX_MOTOR3_CONFIG_ADDR) &&
                         (rx_data[0] < I2C_RX_MOTOR3_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
                    config_index = rx_data[0] - I2C_RX_MOTOR3_CONFIG_ADDR;
                    motor_id     = 2;
                }
                // 配置电机3 0x80_0x8C
                else if ((rx_data[0] >= I2C_RX_MOTOR4_CONFIG_ADDR) &&
                         (rx_data[0] < I2C_RX_MOTOR4_CONFIG_ADDR + I2C_RX_MOTOR_CONFIG_COUNT)) {
                    config_index = rx_data[0] - I2C_RX_MOTOR4_CONFIG_ADDR;
                    motor_id     = 3;
                }
                if (motor_id < 4) {  // 确保索引有效
                    for (int i = 0; i < len - 1; i++) {
                        uint16_t current_config_index = config_index + i;
                        //						if (current_config_index < I2C_RX_MOTOR_CONFIG_COUNT) {
                        uint16_t rx_idx = rx_data[0] - I2C_RX_MOTOR_THROTTLE_BASE + i;
                        if ((current_config_index < I2C_RX_MOTOR_CONFIG_COUNT) && (rx_idx < sizeof(rx_mark)) &&
                            (rx_mark[rx_idx] == 1)) {
                            motor_data.config[motor_id][current_config_index] = rx_buf[rx_idx];
                        }
                        //							if ((current_config_index == 12)) {
                        // printf("motor_id:%d, write addr:0x%02x, data:%d\r\n", motor_id, current_config_index,
                        // rx_buf[rx_idx]);
                        //							}
                        //						}
                    }
                }
            }
        }
        // System Setting
        // 设置A/B编码器模式(0xD0) 和 软启动开关
        else if ((rx_data[0] >= I2C_RX_SYSTEM_SETTINGS_BASE) && (rx_data[0] <= I2C_RX_SYSTEM_SETTINGS_LIMIT)) {
            for (int i = 0; i < len - 1; i++) {
                uint16_t rx_index = rx_data[0] - I2C_RX_SYSTEM_SETTINGS_BASE + i;  // Get Control Register Index
                if (rx_index < sizeof(rx_buf)) {
                    rx_buf[rx_index]  = rx_data[1 + i];
                    rx_mark[rx_index] = 1;
                }
            }
            // set encoder a/b mode
            if (rx_mark[0] == 1) {  // switch A/B mode
                if (motor_data.encoder_ab_mode != rx_buf[0]) {
                    motor_data.encoder_ab_mode     = rx_buf[0];
                    g_events.encoder_abmode_update = 1;
                    g_events.flash_config_update   = 1;
                }
            }
            if (rx_mark[I2C_RX_SOFT_START_AND_STOP - I2C_RX_SYSTEM_SETTINGS_BASE] == 1) {
                if (motor_data.soft_start_stop_switch.value !=
                    rx_buf[I2C_RX_SOFT_START_AND_STOP - I2C_RX_SYSTEM_SETTINGS_BASE]) {
                    motor_data.soft_start_stop_switch.value =
                        rx_buf[I2C_RX_SOFT_START_AND_STOP - I2C_RX_SYSTEM_SETTINGS_BASE];
                    g_events.flash_config_update = 1;
                }
            }
        }
        // set rgb color and brightness(0xC0-0xC4)
        else if ((rx_data[0] >= I2C_RX_RGB_SETTINGS_BASE) && (rx_data[0] <= I2C_RX_RGB_SETTINGS_LIMIT)) {
            for (int i = 0; i < len - 1; i++) {
                uint16_t rx_index = rx_data[0] - I2C_RX_RGB_SETTINGS_BASE + i;
                if (rx_index < sizeof(rx_buf)) {
                    rx_buf[rx_index]  = rx_data[1 + i];
                    rx_mark[rx_index] = 1;
                }
            }
            if (rx_mark[I2C_RX_RGB_SETTINGS_BASE - I2C_RX_RGB_SETTINGS_BASE] == 1) {
                rgb_data.rgb_data.flags.rgb_data_b = rx_buf[I2C_RX_RGB_SETTINGS_BASE - I2C_RX_RGB_SETTINGS_BASE];
            }
            if (rx_mark[(I2C_RX_RGB_SETTINGS_BASE + 1) - I2C_RX_RGB_SETTINGS_BASE] == 1) {
                rgb_data.rgb_data.flags.rgb_data_g = rx_buf[(I2C_RX_RGB_SETTINGS_BASE + 1) - I2C_RX_RGB_SETTINGS_BASE];
            }
            if (rx_mark[(I2C_RX_RGB_SETTINGS_BASE + 2) - I2C_RX_RGB_SETTINGS_BASE] == 1) {
                rgb_data.rgb_data.flags.rgb_data_r = rx_buf[(I2C_RX_RGB_SETTINGS_BASE + 2) - I2C_RX_RGB_SETTINGS_BASE];
            }
            if (rx_mark[(I2C_RX_RGB_SETTINGS_BASE + 4) - I2C_RX_RGB_SETTINGS_BASE] == 1) {
                rgb_data.brightness = rx_buf[(I2C_RX_RGB_SETTINGS_BASE + 4) - I2C_RX_RGB_SETTINGS_BASE];
            }
            if (rx_mark[I2C_RX_RGB_SETTINGS_BASE - I2C_RX_RGB_SETTINGS_BASE] ||
                rx_mark[(I2C_RX_RGB_SETTINGS_BASE + 1) - I2C_RX_RGB_SETTINGS_BASE] ||
                rx_mark[(I2C_RX_RGB_SETTINGS_BASE + 2) - I2C_RX_RGB_SETTINGS_BASE] ||
                rx_mark[(I2C_RX_RGB_SETTINGS_BASE + 4) - I2C_RX_RGB_SETTINGS_BASE]) {
                enqueue_config_request(rgb_data);
                g_events.rgb_update = 1;
            }
        }
        // change I2C slave Address(0xFF)
        else if (rx_data[0] == I2C_RX_I2C_ADDR_CMD_ADDR) {
            if (len == 2) {
                if ((rx_data[1] >= I2C_MIN_ADDR_VALUE) && (rx_data[1] <= I2C_MAX_ADDR_VALUE)) {
                    if (i2c_address[0] != rx_data[1]) {
                        i2c_address[0]                  = rx_data[1];
                        g_events.i2c_addr_config_update = 1;
                        g_events.flash_config_update    = 1;
                    }
                }
            }
        }
        // IAP Update(0xFD)
        else if (rx_data[0] == I2C_RX_JUMP_TO_BOOTLOADER_ADDR) {
            if (rx_data[1] == 1) {
                g_events.jump_to_bootloader = 1;
            }
        }
    }
    // 主机读数据
}

void iap_set(void)
{
    uint8_t i;                                     // Loop index
    uint32_t *pVecTab = (uint32_t *)(0x20000000);  // Pointer to the vector table in SRAM

    // Copy the interrupt vector table from the application address to SRAM
    for (i = 0; i < 48; i++) {
        // Copy each vector entry to the SRAM vector table
        *(pVecTab++) = *(__IO uint32_t *)(APPLICATION_ADDRESS + (i << 2));
    }

    // Enable the SYSCFG peripheral clock
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    // Remap the memory to use SRAM for the vector table
    __HAL_SYSCFG_REMAPMEMORY_SRAM();
}

void read_uid(void)
{
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

    memcpy(&g_uid[0], &uid0, 4);
    memcpy(&g_uid[4], &uid1, 4);
    memcpy(&g_uid[8], &uid2, 4);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    iap_set();
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
    MX_ADC_Init();
    MX_TIM1_Init();
    MX_I2C2_Init();
    MX_IWDG_Init();
    /* USER CODE BEGIN 2 */
    HAL_IWDG_Refresh(&hiwdg);
    init_flash_data();
    user_gpio_init();
    if (HAL_ADCEx_Calibration_Start(&hadc) != HAL_OK) {
        peri_error.flags.adc_setup = 1;
    }
    if (HAL_ADC_Start_DMA(&hadc, (uint32_t *)adc_data.mirrorAdcValueBuf, ADC_DMA_BUFFER_LENGTH) != HAL_OK) {
        peri_error.flags.adc_setup = 1;
    }
    HAL_IWDG_Refresh(&hiwdg);
    sk6812_init(1);
    InitMotorPwm();
    InitMotorEncoder();
    init_motor();
    init_pt_task();
    user_i2c_init();
    i2c2_it_enable();

    rgb_data.rgb_data.value = 0;
    rgb_data.brightness     = 20;
    neopixel_set_color_brightness(0, rgb_data.rgb_data.value, rgb_data.brightness);
    neopixel_show();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        main_scheduler(&main_pt);
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

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState            = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL12;
    RCC_OscInitStruct.PLL.PREDIV          = RCC_PREDIV_DIV1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
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

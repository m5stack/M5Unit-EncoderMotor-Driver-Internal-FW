#include "pt_task.h"
#include "i2c.h"
#include "gpio.h"
#include "motorencoder.h"
#include "motorbdc.h"
#include "pid.h"
#include "sys_utils.h"
#include "adc.h"
#include "ws2812.h"
#include "flash.h"
#include <math.h>
#include <string.h>
#include "power.h"
#include "iwdg.h"
#include "stm32f0xx_ll_adc.h"

#define SPEED_THROTTLE_STEP_LIMIT     10     // 速度模式单周期最大油门变化
#define POS_THROTTLE_STEP_LIMIT       10     // 位置模式单周期最大油门变化
#define SPEED_MIN_REVERSE_ZERO_V      8      // 速度模式换向保护死区（脉冲）
#define POS_MIN_REVERSE_ZERO_V        2      // 位置模式换向保护死区（脉冲）
#define POS_SPEED_MULT_OVERFLOW_GUARD 10000  // 防止乘法溢出的阈值

#define POS_DEADZONE           0   // 位置死区：±1个编码器脉冲
#define POS_APPROACH_ZONE      50  // 接近区域：50个编码器脉冲
#define POS_APPROACH_MAX_SPEED 30  // 接近区最大速度
#define POS_MIN_REVERSE_SPEED  10  // 最小触发换向的速度阈值

// 用于油门平滑各自独立变量
#define POS_APPROACH_MIN_SPEED     6
#define POS_ON_TARGET_STABLE_TICKS 50
#define POS_ZERO_CROSS_BRAKE_GAIN  8
#define POS_ZERO_CROSS_BRAKE_LIMIT 30

#define ADC_STEP_CONFIRM_DELTA   100
#define ADC_STEP_CONFIRM_TICKS   3
#define ADC_LOW_PASS_ALPHA       0.10f
#define ADC_LARGE_STEP_ALPHA     0.35f
#define ADC_INA_AMP_GAIN         50.0f
#define ADC_INA_SHUNT_OHM        0.01f
#define ADC_INA_CURRENT_CAL_GAIN 1.0f

static int8_t speed_last_throttle[4] = {0};

static int8_t pos_last_throttle[4] = {0};  // 油门斜坡平滑变量
static int32_t last_encoder[4]     = {0};  // 上一次编码器值
static uint8_t reverse_lock[4]     = {0};  // 换向锁定标志

static uint32_t reverse_start_time[4] = {0};
static uint8_t reverse_state[4]       = {0};  // 0=正常运行, 1=等待换向停止
static int32_t target_position[4]     = {0};
static int32_t pos_out_data[4]        = {0};

static volatile uint32_t rgb_set_timeout = 0;

static inline int32_t motor_axis_to_throttle(int32_t axis_output)
{
    return motor_data.encoder_ab_mode ? -axis_output : axis_output;
}

static inline int32_t i32_abs(int32_t value)
{
    return value < 0 ? -value : value;
}

static inline void reset_pos_pid_state(volatile pid_pos_t *pid, int32_t last_error)
{
    pid->i_sum = 0;
    pid->ek1   = last_error;
}

struct pt main_pt;
struct pt i2c_slave_timeout_handle_pt;
struct pt speed_loop_handle_pt;
struct pt pos_loop_handle_pt;
struct pt motor_pwm_update_handle_pt;
struct pt motor_soft_start_and_stop_handle_pt;
struct pt adc_to_voltage_handle_pt;
struct pt rgb_update_handle_pt;
struct pt i2c_addr_config_setter_pt;
struct pt flash_config_setter_pt;
struct pt rgb_update_timeout_handle_pt;
struct pt jump_to_bootloader_setter_pt;
struct pt stm_feed_wdt_handle_pt;

void init_pt_task(void)
{
    PT_INIT(&main_pt);
    PT_INIT(&i2c_slave_timeout_handle_pt);
    PT_INIT(&speed_loop_handle_pt);
    PT_INIT(&pos_loop_handle_pt);
    PT_INIT(&motor_pwm_update_handle_pt);
    PT_INIT(&motor_soft_start_and_stop_handle_pt);
    PT_INIT(&adc_to_voltage_handle_pt);
    PT_INIT(&rgb_update_handle_pt);
    PT_INIT(&i2c_addr_config_setter_pt);
    PT_INIT(&flash_config_setter_pt);
    PT_INIT(&rgb_update_timeout_handle_pt);
    PT_INIT(&jump_to_bootloader_setter_pt);
    PT_INIT(&stm_feed_wdt_handle_pt);
}

static PT_THREAD(stm_feed_wdt_handle(struct pt *pt))
{
    PT_BEGIN(pt);

    while (1) {
        PT_TIMER_DELAY(pt, PT_MILLIS_SECOND(10));
        HAL_IWDG_Refresh(&hiwdg);
    }

    PT_END(pt);
}

static PT_THREAD(jump_to_bootloader_setter(struct pt *pt))
{
    PT_BEGIN(pt);
    while (1) {
        PT_WAIT_UNTIL(pt, g_events.jump_to_bootloader == 1);  // 等待设置触发信号
        g_events.jump_to_bootloader = 0;                      // 清除标志

        Jump_Bootloader();

        PT_YIELD(pt);  // 主动让出CPU
    }
    PT_END(pt);
}

static PT_THREAD(i2c_addr_config_setter(struct pt *pt))
{
    PT_BEGIN(pt);
    while (1) {
        PT_WAIT_UNTIL(pt, g_events.i2c_addr_config_update == 1);  // 等待设置触发信号
        g_events.i2c_addr_config_update = 0;                      // 清除标志

        __disable_irq();  // 屏蔽所有中断

        user_i2c_init();

        __enable_irq();  // 打开所有中断
    }
    PT_END(pt);
}

PT_THREAD(flash_config_setter(struct pt *pt))
{
    PT_BEGIN(pt);
    while (1) {
        PT_WAIT_UNTIL(pt, g_events.flash_config_update == 1);  // 等待设置触发信号
        g_events.flash_config_update = 0;                      // 清除标志

        __disable_irq();  // 屏蔽所有中断

        flash_data_write_back();

        __enable_irq();                             // 打开所有中断
        if (g_events.encoder_abmode_update == 1) {  // 如果ab模式切换，直接重启系统
            g_events.encoder_abmode_update = 0;
            HAL_NVIC_SystemReset();
        }
    }
    PT_END(pt);
}

static PT_THREAD(rgb_update_handle(struct pt *pt))
{
    static rgb_data_t pending_rgb;

    PT_BEGIN(pt);
    while (1) {
        PT_WAIT_UNTIL(pt, config_queue_has_data() && !g_events.rgb_busy);
        dequeue_config_request(&pending_rgb);

        if (!g_events.rgb_busy) {
            g_events.rgb_busy = 1;
            neopixel_set_color_brightness(0, pending_rgb.rgb_data.value, pending_rgb.brightness);
            neopixel_show();
            rgb_set_timeout = HAL_GetTick();
        }
    }
    PT_END(pt);
}

static PT_THREAD(rgb_update_timeout_handle(struct pt *pt))
{
    PT_BEGIN(pt);
    while (1) {
        if (g_events.rgb_busy && (HAL_GetTick() - rgb_set_timeout >= RGB_TIMEOUT)) {
            g_events.rgb_busy = 0;
        }
        PT_YIELD(pt);
    }
    PT_END(pt);
}

static PT_THREAD(adc_to_voltage_handle(struct pt *pt))
{
    // 使用独立的中值和均值缓冲区
    static uint16_t median_buf[ADC_CHANNEL_NUMS][MEDIAN_WINDOW] = {0};
    static uint8_t median_idx[ADC_CHANNEL_NUMS]                 = {0};
    static uint16_t confirmed_adc[ADC_CHANNEL_NUMS]             = {0};
    static uint16_t candidate_adc[ADC_CHANNEL_NUMS]             = {0};
    static uint8_t candidate_ticks[ADC_CHANNEL_NUMS]            = {0};
    static uint8_t adc_filter_init[ADC_CHANNEL_NUMS]            = {0};
    static float adc_filtered[ADC_CHANNEL_NUMS]                 = {0.0f};
    static uint16_t averaged_adc[ADC_CHANNEL_NUMS]              = {0};

    PT_BEGIN(pt);
    while (1) {
        PT_WAIT_UNTIL(pt, adc_data.adc_average_ready != 0U);

        __disable_irq();
        for (int i = 0; i < ADC_CHANNEL_NUMS; i++) {
            averaged_adc[i] = adc_data.averagedAdcValueBuf[i];
        }
        adc_data.adc_average_ready = 0U;
        __enable_irq();

        for (int i = 0; i < ADC_CHANNEL_NUMS; i++) {
            // 原始读取
            uint16_t dma_raw_val = averaged_adc[i];
            if (dma_raw_val > 4095) {
                dma_raw_val = 4095;
            }

            if (i == ADC_INA_CHANNEL_INDEX) {
                adc_filtered[i]           = (float)dma_raw_val;
                adc_filter_init[i]        = 1;
                adc_data.uiAdcValueBuf[i] = (uint32_t)(adc_filtered[i] + 0.5f);
                continue;
            }
            // Step 2: 中值滤波（抗脉冲）
            uint16_t median_val = median_filter(dma_raw_val, median_buf[i], &median_idx[i]);

            // 再移动平均平滑
            if (!adc_filter_init[i]) {
                confirmed_adc[i]   = median_val;
                candidate_adc[i]   = median_val;
                adc_filtered[i]    = (float)median_val;
                candidate_ticks[i] = 0;
                adc_filter_init[i] = 1;
            } else {
                uint16_t diff =
                    (median_val > confirmed_adc[i]) ? (median_val - confirmed_adc[i]) : (confirmed_adc[i] - median_val);

                if (diff <= ADC_STEP_CONFIRM_DELTA) {
                    confirmed_adc[i]   = median_val;
                    candidate_adc[i]   = median_val;
                    candidate_ticks[i] = 0;
                    adc_filtered[i] += ADC_LOW_PASS_ALPHA * ((float)confirmed_adc[i] - adc_filtered[i]);
                } else {
                    uint16_t candidate_diff = (median_val > candidate_adc[i]) ? (median_val - candidate_adc[i])
                                                                              : (candidate_adc[i] - median_val);

                    if (candidate_diff <= ADC_STEP_CONFIRM_DELTA) {
                        candidate_adc[i] = median_val;
                        if (candidate_ticks[i] < ADC_STEP_CONFIRM_TICKS) {
                            candidate_ticks[i]++;
                        }
                    } else {
                        candidate_adc[i]   = median_val;
                        candidate_ticks[i] = 1;
                    }

                    if (candidate_ticks[i] >= ADC_STEP_CONFIRM_TICKS) {
                        confirmed_adc[i]   = candidate_adc[i];
                        candidate_ticks[i] = 0;
                        adc_filtered[i] += ADC_LARGE_STEP_ALPHA * ((float)confirmed_adc[i] - adc_filtered[i]);
                    }
                }
            }

            adc_data.uiAdcValueBuf[i] = (uint32_t)(adc_filtered[i] + 0.5f);
        }

        //        printf("ina channel adc: %u\r\n", adc_data.uiAdcValueBuf[1]);

        // 使用内部基准电压计算实际VDD电压
        // 通道3是内部基准电压 (VREFINT)
        // 计算实际VDD电压 (mV)
        adc_data.ref_voltage =
            __LL_ADC_CALC_VREFANALOG_VOLTAGE(adc_data.uiAdcValueBuf[2], ADC_RESOLUTION_12B);  // REFVINT

        if (!adc_data.is_ina_ref_init && adc_data.ref_voltage >= 3000 && adc_data.ref_voltage <= 3600) {
            ++adc_data.ina_ref_cal_times;
            if (adc_data.ina_ref_cal_times >= INA_REF_CAL_TIMES) {
                adc_data.is_ina_ref_init = 1;
            }
        }

        if (adc_data.is_ina_ref_init) {
            // 计算电压 - 使用实际测量的VDD
#if 0
           adc_data.voltage_vm_mv = (uint32_t)adc_data.uiAdcValueBuf[1] * adc_data.ref_voltage / 4095 * 6;
            // 计算差分电压
            int32_t ina_adc_diff = adc_data.uiAdcValueBuf[2] - adc_data.uiAdcValueBuf[0];  	// 需要实际的外部参考电压值计算
            adc_data.vout_diff = (float)ina_adc_diff * (adc_data.ref_voltage / 1000.0f) / 4095.0f;
#else
            adc_data.voltage_vm_mv =
                __LL_ADC_CALC_DATA_TO_VOLTAGE(adc_data.ref_voltage, (uint32_t)adc_data.uiAdcValueBuf[0],
                                              ADC_RESOLUTION_12B) *
                6;
            // 计算差分电压
            adc_data.vout_diff = adc_filtered[ADC_INA_CHANNEL_INDEX] * (adc_data.ref_voltage / 1000.0f) / 4095.0f;
#endif

            // 计算电流 (分流电阻为0.01Ω)
            float raw_current = adc_data.vout_diff / (ADC_INA_AMP_GAIN * ADC_INA_SHUNT_OHM);

            raw_current *= ADC_INA_CURRENT_CAL_GAIN;
            adc_data.current_ina_ma = (int32_t)(raw_current * 1000.0f);
            adc_data.i_in_value     = raw_current;
        }
    }
    PT_END(pt);
}

// ------------ 1. 速度模式线程 ------------
static PT_THREAD(speed_loop_handle(struct pt *pt))
{
    static int32_t encoder_last[4] = {0};

    PT_BEGIN(pt);
    while (1) {
        PT_TIMER_DELAY(pt, PT_MILLIS_SECOND(20));
        for (int i = 0; i < 4; i++) {
            int32_t encoder_speed = encoder_raw[i].encoder_raw.value - encoder_last[i];
            int32_t control_speed = motor_data.encoder_ab_mode ? -encoder_speed : encoder_speed;

            // Keep the protocol speed value as the raw encoder count delta.
            motor_data.speed[i] = encoder_speed;
            encoder_last[i]     = encoder_raw[i].encoder_raw.value;

            int32_t out_data = 0;
            if (motor_data.config[i][MOTOR_CONFIG_MOTOR_MODE] == 0x02) {
                if ((*(int8_t *)&motor_data.config[i][MOTOR_CONFIG_MOTOR_SPEED_POINT] == motor_data.speed[i]) &&
                    (*(int8_t *)&motor_data.config[i][MOTOR_CONFIG_MOTOR_SPEED_POINT] == 0)) {
                    out_data                     = 0;
                    motor_data.motor_throttle[i] = 0;
                } else {
                    out_data = PIDPos(&pid_speed[i],
                                      // *(int8_t *)&motor_data.config[i][MOTOR_CONFIG_MOTOR_SPEED_POINT],
                                      // motor_data.speed[i]) / 10;
                                      *(int8_t *)&motor_data.config[i][MOTOR_CONFIG_MOTOR_SPEED_POINT], control_speed) /
                               5;
                    out_data                     = CONSTRAIN(out_data, -127, 127);
                    motor_data.motor_throttle[i] = CONSTRAIN(out_data + motor_data.motor_throttle[i], -127, 127);
                }
            }
        }
    }
    PT_END(pt);
}

// ------------ 2. 位置模式线程 ------------
static PT_THREAD(pos_loop_handle(struct pt *pt))
{
    PT_BEGIN(pt);

    while (1) {
        PT_TIMER_DELAY(pt, PT_MILLIS_SECOND(1));

        for (int i = 0; i < 4; i++) {
            if (motor_data.config[i][MOTOR_CONFIG_MOTOR_MODE] == 0x01) {  // 位置控制模式
                // 目标位置读取
                memcpy(&target_position[i], &motor_data.config[i][MOTOR_CONFIG_MOTOR_POS_POINT_BYTE0], 4);
                int32_t curr_pos = encoder_raw[i].encoder_raw.value;

                // 单周期速度估算
                int32_t curr_speed = curr_pos - last_encoder[i];
                last_encoder[i]    = curr_pos;

                // 当前误差
                int32_t pos_error = target_position[i] - curr_pos;
                int32_t abs_error = abs(pos_error);

                // ========== 1. 死区判断：已到达目标 ==========
                if (abs_error == 0) {
                    motor_data.motor_throttle[i] = 0;
                    pos_last_throttle[i]         = 0;
                    reverse_state[i]             = 0;
                    // 清除PID积分
                    pid_pos[i].i_sum = 0;
                    continue;
                }

                // ========== 2. 处理换向等待状态 ==========
                if (reverse_state[i] == 1) {
                    // 强制油门归零（斜坡减速）
                    int delta = 0 - pos_last_throttle[i];
                    if (delta > POS_THROTTLE_STEP_LIMIT) delta = POS_THROTTLE_STEP_LIMIT;
                    if (delta < -POS_THROTTLE_STEP_LIMIT) delta = -POS_THROTTLE_STEP_LIMIT;
                    pos_last_throttle[i] += delta;
                    motor_data.motor_throttle[i] = motor_axis_to_throttle(pos_last_throttle[i]);

                    // 检查是否已等待200ms 或 已经完全停止
                    uint32_t elapsed = HAL_GetTick() - reverse_start_time[i];
                    if (elapsed >= 200 || abs(pos_last_throttle[i]) < 3) {
                        // 清除状态，准备重新开始
                        reverse_state[i]             = 0;
                        pos_last_throttle[i]         = 0;
                        motor_data.motor_throttle[i] = 0;
                        // 重置PID积分
                        pid_pos[i].i_sum = 0;
                    }
                    HAL_IWDG_Refresh(&hiwdg);
                    continue;
                }

                // ========== 3. 换向检测（仅在高速时触发）==========
                // 条件：误差和速度方向相反 + 速度超过阈值 + 不在接近区域
                if ((pos_error * curr_speed < 0) && (abs(curr_speed) > POS_MIN_REVERSE_SPEED) &&
                    (abs_error > POS_APPROACH_ZONE)) {
                    // 进入换向等待状态
                    reverse_state[i]      = 1;
                    reverse_start_time[i] = HAL_GetTick();
                    continue;
                }

                // ========== 4. PID控制 ==========
                pos_out_data[i] = PIDPos(&pid_pos[i], target_position[i], curr_pos);

                // ========== 5. 接近区域速度限制 ==========
                int32_t max_speed = motor_data.config[i][MOTOR_CONFIG_MOTOR_POS_MAX_SPEED];

                if (abs_error < POS_APPROACH_ZONE) {
                    // 在接近区域，根据误差动态调整最大速度
                    // 误差越小，速度上限越低
                    int32_t approach_limit = (abs_error * POS_APPROACH_MAX_SPEED) / POS_APPROACH_ZONE;
                    if (approach_limit < 10) approach_limit = 10;  // 最小速度限制

                    if (max_speed > approach_limit) {
                        max_speed = approach_limit;
                    }
                }

                // 油门限幅
                pos_out_data[i] = CONSTRAIN(pos_out_data[i], -max_speed, max_speed);

                // ========== 6. 油门斜坡平滑输出 ==========
                int delta = pos_out_data[i] - pos_last_throttle[i];
                if (delta > POS_THROTTLE_STEP_LIMIT) delta = POS_THROTTLE_STEP_LIMIT;
                if (delta < -POS_THROTTLE_STEP_LIMIT) delta = -POS_THROTTLE_STEP_LIMIT;
                pos_last_throttle[i] += delta;
                motor_data.motor_throttle[i] = motor_axis_to_throttle(pos_last_throttle[i]);
                HAL_IWDG_Refresh(&hiwdg);
            }
        }
    }
    PT_END(pt);
}

// ------------ 3. PWM硬件输出线程 ------------
static PT_THREAD(motor_pwm_update_handle(struct pt *pt))
{
    PT_BEGIN(pt);
    while (1) {
        for (int i = 0; i < 4; i++) {
            SetMotorThrottle(i, motor_data.motor_throttle[i]);
        }
        PT_YIELD(pt);  // 可选，为了调度
    }
    PT_END(pt);
}

static PT_THREAD(motor_soft_start_and_stop_handle(struct pt *pt))
{
    PT_BEGIN(pt);
    //	    while(1) {
    //	    	ProcessSoftStartUpdate();
    //	    	PT_YIELD(pt);
    //	    }
    PT_END(pt);
}

static PT_THREAD(i2c_slave_timeout_handle(struct pt *pt))
{
    static uint32_t hal_get_tick_temp = 0;

    PT_BEGIN(pt);
    while (1) {
        if (i2c_slave_state == I2C_SLAVE_STATE_ERROR) {
            i2c2_reset();
            i2c_slave_state = I2C_SLAVE_STATE_IDLE;
        } else if (i2c_slave_state != I2C_SLAVE_STATE_IDLE) {
            hal_get_tick_temp = HAL_GetTick();
            if ((hal_get_tick_temp - i2c_ex_timeout_start) > I2C_TIMEOUT_MS) {
                i2c2_reset();
                i2c_slave_state = I2C_SLAVE_STATE_IDLE;
            }
        }
        PT_YIELD(pt);  // 主动让出CPU
    }
    PT_END(pt);
}

void main_scheduler(struct pt *pt)
{
    PT_BEGIN(pt);
    while (1) {
        // 按优先级顺序调度
        PT_SCHEDULE(stm_feed_wdt_handle(&stm_feed_wdt_handle_pt));
        PT_SCHEDULE(pos_loop_handle(&pos_loop_handle_pt));
        PT_SCHEDULE(speed_loop_handle(&speed_loop_handle_pt));
        PT_SCHEDULE(motor_pwm_update_handle(&motor_pwm_update_handle_pt));
        PT_SCHEDULE(motor_soft_start_and_stop_handle(&motor_soft_start_and_stop_handle_pt));
        PT_SCHEDULE(rgb_update_timeout_handle(&rgb_update_timeout_handle_pt));
        PT_SCHEDULE(rgb_update_handle(&rgb_update_handle_pt));
        PT_SCHEDULE(adc_to_voltage_handle(&adc_to_voltage_handle_pt));
        PT_SCHEDULE(i2c_addr_config_setter(&i2c_addr_config_setter_pt));
        PT_SCHEDULE(flash_config_setter(&flash_config_setter_pt));
        PT_SCHEDULE(i2c_slave_timeout_handle(&i2c_slave_timeout_handle_pt));
        PT_SCHEDULE(jump_to_bootloader_setter(&jump_to_bootloader_setter_pt));
    }
    PT_END(pt);
}

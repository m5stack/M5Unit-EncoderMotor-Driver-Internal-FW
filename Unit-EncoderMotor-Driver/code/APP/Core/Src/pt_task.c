#include "pt_task.h"
#include "i2c.h"
#include "gpio.h"
#include "mymotorencoder.h"
#include "mymotorbdc.h"
#include "pid.h"
#include "sys_utils.h"
#include "adc.h"
#include "ws2812.h"
#include "flash.h"
#include <math.h>
#include <string.h>
#include "power.h"
#include "iwdg.h"

static volatile uint32_t rgb_set_timeout = 0;

static inline int32_t motor_axis_to_throttle(int32_t axis_output)
{
    return motor_data.encoder_ab_mode ? -axis_output : axis_output;
}

static inline int32_t throttle_to_motor_axis(int32_t throttle)
{
    return motor_data.encoder_ab_mode ? -throttle : throttle;
}

#define POS_APPROACH_RANGE_COUNTS  80
#define POS_MIN_TRACK_THROTTLE     10
#define POS_ZERO_CROSS_BRAKE_GAIN  16
#define POS_ZERO_CROSS_BRAKE_LIMIT 40
#define POS_OUTPUT_ACCEL_STEP      2
#define POS_OUTPUT_BRAKE_STEP      8
#define POS_ON_TARGET_STABLE_TICKS 4

static inline int32_t abs_i32(int32_t value)
{
    if (value == INT32_MIN) {
        return INT32_MAX;
    }
    return (value < 0) ? -value : value;
}

static inline int32_t subtract_i32_saturated(int32_t left, int32_t right)
{
    int64_t result = (int64_t)left - (int64_t)right;

    if (result > INT32_MAX) {
        return INT32_MAX;
    }
    if (result < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)result;
}

static inline int32_t sign_i32(int32_t value)
{
    return (value > 0) - (value < 0);
}

static inline void reset_pos_pid_state(volatile pid_pos_t *pid, int32_t error)
{
    pid->i_sum = 0;
    pid->ek1   = error;
}

static int32_t pos_dynamic_throttle_limit(int32_t error, int32_t max_speed)
{
    int32_t abs_error = abs_i32(error);

    max_speed = abs_i32(max_speed);
    max_speed = CONSTRAIN(max_speed, 0, 127);
    if (max_speed <= POS_MIN_TRACK_THROTTLE || abs_error >= POS_APPROACH_RANGE_COUNTS) {
        return max_speed;
    }

    int32_t limit =
        POS_MIN_TRACK_THROTTLE + (abs_error * (max_speed - POS_MIN_TRACK_THROTTLE)) / POS_APPROACH_RANGE_COUNTS;

    return CONSTRAIN(limit, POS_MIN_TRACK_THROTTLE, max_speed);
}

static int32_t pos_limit_output_step(int32_t target_output, int32_t last_output)
{
    int32_t delta = target_output - last_output;
    int32_t step  = POS_OUTPUT_ACCEL_STEP;

    if ((target_output == 0) || ((last_output != 0) && (sign_i32(target_output) != sign_i32(last_output))) ||
        (abs_i32(target_output) < abs_i32(last_output))) {
        step = POS_OUTPUT_BRAKE_STEP;
    }

    if (delta > step) {
        return last_output + step;
    }
    if (delta < -step) {
        return last_output - step;
    }

    return target_output;
}

struct pt main_pt;
struct pt i2c_slave_timeout_handle_pt;
struct pt speed_loop_handle_pt;
struct pt pos_loop_handle_pt;
struct pt motor_pwm_update_handle_pt;
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

        __enable_irq();  // 打开所有中断

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
            neopixel_set_color(0, pending_rgb.rgb_data.value, pending_rgb.brightness);
            ws2812_show();
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
    static uint16_t avg_buf[ADC_CHANNEL_NUMS][FILTER_WINDOW]    = {0};
    static uint8_t avg_idx[ADC_CHANNEL_NUMS]                    = {0};
    static uint16_t last_adc_val[ADC_CHANNEL_NUMS]              = {0};
    static float filtered_current                               = 0.0f;

    PT_BEGIN(pt);
    while (1) {
        PT_TIMER_MICRODELAY(pt, 50);

        for (int i = 0; i < ADC_CHANNEL_NUMS; i++) {
            // 原始读取
            uint16_t raw_val = safe_adc_read(adc_data.uiAdcValueBuf[i], last_adc_val[i]);
            last_adc_val[i]  = raw_val;

            // Step 2: 中值滤波（抗脉冲）
            uint16_t median_val = median_filter(raw_val, median_buf[i], &median_idx[i]);

            // 再移动平均平滑
            adc_data.uiAdcValueBuf[i] = moving_average(median_val, avg_buf[i], &avg_idx[i]);
        }

        adc_data.ref_voltage = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(adc_data.uiAdcValueBuf[2], ADC_RESOLUTION_12B);
        if (!adc_data.is_ina_ref_init && adc_data.ref_voltage >= 3000 && adc_data.ref_voltage <= 3600) {
            ++adc_data.ina_ref_cal_times;
            if (adc_data.ina_ref_cal_times >= INA_REF_CAL_TIMES) {
                adc_data.is_ina_ref_init = 1;
            }
        }

        if (adc_data.is_ina_ref_init) {
            adc_data.voltage_vm_mv = adc_data.uiAdcValueBuf[1] * adc_data.ref_voltage / 4095 * 6;
            if (adc_data.voltage_vm_mv <= 600) {
                adc_data.voltage_vm_mv = 0;
            }
            int32_t ina_adc_diff = adc_data.uiAdcValueBuf[0] - adc_data.uiAdcValueBuf[3];
            //            adc_data.vout_diff = ina_adc_diff * (adc_data.ref_voltage / 1000.0f) / 4095;
            float ina_adc_diff_f = ina_adc_diff - adc_data.offset_val;
            adc_data.vout_diff   = ina_adc_diff_f * (adc_data.ref_voltage / 1000.0f) / 4095;
            //            printf("VM_mV:%lu, ref_volt:%lu, adc_ina:%lu, adc_ref:%lu\r\n",
            //            		adc_data.voltage_vm_mv, adc_data.ref_voltage, adc_data.uiAdcValueBuf[0],
            //            adc_data.uiAdcValueBuf[3]);

            float raw_current = adc_data.vout_diff / 0.5f;

            // 带抗饱和处理的低通滤波
            float delta = fabsf(raw_current - filtered_current);
            float adaptive_alpha;

            if (delta < 1.0f) {          // 小波动区间
                adaptive_alpha = 0.05f;  // 强滤波抑噪
            } else if (delta < 5.0f) {   // 中等波动
                                         //                adaptive_alpha = 0.2f;
                adaptive_alpha = 0.5f;
            } else {                    // 大信号变化
                                        //                adaptive_alpha = 0.7f;    // 弱滤波快速跟踪
                adaptive_alpha = 0.9f;  // 弱滤波快速跟踪
            }
            filtered_current = adaptive_alpha * raw_current + (1 - adaptive_alpha) * filtered_current;
            //            adc_data.current_ina_ma = (int32_t)(filtered_current * 1000);
            int32_t ina_ma = (int32_t)(filtered_current * 1000);
            if ((ina_ma > -5) && (ina_ma < 5)) {
                adc_data.current_ina_ma = 0;
                adc_data.i_in_value     = 0;
            } else {
                adc_data.current_ina_ma = (int32_t)(filtered_current * 1000);
                adc_data.i_in_value     = filtered_current;
            }
        }
    }
    PT_END(pt);
}

static PT_THREAD(speed_loop_handle(struct pt *pt))
{
    static int32_t encoder_last[4] = {0};

    PT_BEGIN(pt);
    while (1) {
        PT_TIMER_DELAY(pt, PT_MILLIS_SECOND(20));
        int32_t encoder_now   = encoder_raw[MOTOR_M1].encoder_raw.value;
        int32_t raw_speed     = encoder_now - encoder_last[MOTOR_M1];
        int32_t control_speed = motor_data.encoder_ab_mode ? -raw_speed : raw_speed;

        // Keep the protocol speed value as the raw encoder count delta.
        motor_data.speed[MOTOR_M1] = raw_speed;
        encoder_last[MOTOR_M1]     = encoder_now;

        int32_t out_data = 0;
        if (motor_data.config[MOTOR_M1][MOTOR_CONFIG_MOTOR_MODE] == 0x02) {
            if ((*(int8_t *)&motor_data.config[MOTOR_M1][MOTOR_CONFIG_MOTOR_SPEED_POINT] == raw_speed) &&
                (*(int8_t *)&motor_data.config[MOTOR_M1][MOTOR_CONFIG_MOTOR_SPEED_POINT] == 0)) {
                out_data                            = 0;
                motor_data.motor_throttle[MOTOR_M1] = 0;
            } else {
                out_data =
                    PIDPos(&pid_speed[MOTOR_M1],
                           *(int8_t *)&motor_data.config[MOTOR_M1][MOTOR_CONFIG_MOTOR_SPEED_POINT], control_speed) /
                    5;
                out_data = CONSTRAIN(out_data, -127, 127);
                motor_data.motor_throttle[MOTOR_M1] =
                    CONSTRAIN(out_data + motor_data.motor_throttle[MOTOR_M1], -127, 127);
            }
        }
    }
    PT_END(pt);
}

static PT_THREAD(pos_loop_handle(struct pt *pt))
{
    static uint8_t pos_active_last        = 0;
    static uint8_t on_target_stable_ticks = 0;
    static int32_t last_target_pos        = 0;
    static int32_t last_encoder_pos       = 0;
    static int32_t last_axis_throttle     = 0;

    PT_BEGIN(pt);
    while (1) {
        PT_TIMER_DELAY(pt, PT_MILLIS_SECOND(1));

        int32_t out_data = 0;
        if (motor_data.config[MOTOR_M1][MOTOR_CONFIG_MOTOR_MODE] == 0x01) {
            int32_t target_pos        = *(int32_t *)&motor_data.config[MOTOR_M1][MOTOR_CONFIG_MOTOR_POS_POINT_BYTE0];
            int32_t encoder_pos       = encoder_raw[MOTOR_M1].encoder_raw.value;
            int32_t pos_error         = subtract_i32_saturated(target_pos, encoder_pos);
            int32_t encoder_delta     = pos_active_last ? (encoder_pos - last_encoder_pos) : 0;
            int32_t max_axis_throttle = *(int8_t *)&motor_data.config[MOTOR_M1][MOTOR_CONFIG_MOTOR_POS_MAX_SPEED];
            int32_t axis_throttle     = 0;

            if (!pos_active_last || (target_pos != last_target_pos)) {
                reset_pos_pid_state(&pid_pos[MOTOR_M1], pos_error);
                on_target_stable_ticks = 0;
            }

            if (pos_error == 0) {
                if (encoder_delta == 0) {
                    if (on_target_stable_ticks < POS_ON_TARGET_STABLE_TICKS) {
                        on_target_stable_ticks++;
                    }
                } else {
                    on_target_stable_ticks = 0;
                }

                if (on_target_stable_ticks >= POS_ON_TARGET_STABLE_TICKS) {
                    axis_throttle      = 0;
                    last_axis_throttle = 0;
                    reset_pos_pid_state(&pid_pos[MOTOR_M1], 0);
                } else {
                    axis_throttle = CONSTRAIN(-encoder_delta * POS_ZERO_CROSS_BRAKE_GAIN, -POS_ZERO_CROSS_BRAKE_LIMIT,
                                              POS_ZERO_CROSS_BRAKE_LIMIT);
                }
            } else {
                on_target_stable_ticks = 0;
                out_data               = PIDPos(&pid_pos[MOTOR_M1], target_pos, encoder_pos);

                max_axis_throttle = pos_dynamic_throttle_limit(pos_error, max_axis_throttle);
                axis_throttle     = CONSTRAIN(out_data, -max_axis_throttle, max_axis_throttle);
            }

            axis_throttle                       = pos_limit_output_step(axis_throttle, last_axis_throttle);
            last_axis_throttle                  = axis_throttle;
            motor_data.motor_throttle[MOTOR_M1] = motor_axis_to_throttle(axis_throttle);

            last_target_pos  = target_pos;
            last_encoder_pos = encoder_pos;
            pos_active_last  = 1;
        } else {
            if (pos_active_last) {
                reset_pos_pid_state(&pid_pos[MOTOR_M1], 0);
                on_target_stable_ticks = 0;
                last_axis_throttle     = 0;
                pos_active_last        = 0;
            }
            last_encoder_pos = encoder_raw[MOTOR_M1].encoder_raw.value;
        }
    }
    PT_END(pt);
}

static PT_THREAD(motor_pwm_update_handle(struct pt *pt))
{
    PT_BEGIN(pt);
    while (1) {
        SetMotorThrottle(PWM_M1, motor_data.motor_throttle[MOTOR_M1]);
        PT_YIELD(pt);
    }
    PT_END(pt);
}

static PT_THREAD(i2c_slave_timeout_handle(struct pt *pt))
{
    static uint32_t hal_get_tick_temp = 0;

    PT_BEGIN(pt);
    while (1) {
        if (i2c_slave_state != I2C_SLAVE_STATE_IDLE) {
            hal_get_tick_temp = HAL_GetTick();
            if ((hal_get_tick_temp > i2c_ex_timeout_start) &&
                (hal_get_tick_temp - i2c_ex_timeout_start > I2C_TIMEOUT_MS)) {
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

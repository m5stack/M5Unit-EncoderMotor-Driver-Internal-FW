#include "mymotorbdc.h"

#include "tim.h"

#include "sys_utils.h"

#include <stdlib.h>

#define M1R_H LL_GPIO_SetOutputPin(MCU_DIR_M1R_GPIO_Port, MCU_DIR_M1R_Pin)
#define M1R_L LL_GPIO_ResetOutputPin(MCU_DIR_M1R_GPIO_Port, MCU_DIR_M1R_Pin)
#define M1F_H LL_GPIO_SetOutputPin(MCU_DIR_M1F_GPIO_Port, MCU_DIR_M1F_Pin)
#define M1F_L LL_GPIO_ResetOutputPin(MCU_DIR_M1F_GPIO_Port, MCU_DIR_M1F_Pin)

static uint32_t normal_motor_pid_velocity_output_ramp = 100;
static int16_t setpoint[4] = {0}, last_setpoint[4] = {0};
static int16_t output[4] = {0}, output_prev[4] = {0};
static uint32_t my_timestamp_prev[4] = {0};
static uint8_t is_bootup[4]          = {0};

void SetMotorDir(uint8_t port, uint8_t dir)
{
    switch (port) {
        case PWM_M1: {
            if (!dir) {
                M1F_H;
                M1R_L;
            } else {
                M1R_H;
                M1F_L;
            }
            break;
        }
        default: {
            break;
        }
    }
}

void SetMotorPwm(uint8_t port, uint16_t value)
{
    switch (port) {
        case PWM_M1: {
            TIM16->CCR1 = value;
            break;
        }
        default: {
            break;
        }
    }
}

// 声明辅助函数（放在文件顶部）
static inline void setMotorOutput(uint8_t port, int16_t output_val)
{
    if (output_val > 0) {
        SetMotorDir(port, MDIR_FFW);
        SetMotorPwm(port, (uint16_t)(output_val) * 255u / 127);
    } else {
        SetMotorDir(port, MDIR_REV);
        SetMotorPwm(port, (uint16_t)(-output_val) * 255u / 127);
    }
}

void SetMotorThrottle(uint8_t port, int8_t throttle)
{
    // 检查是否启用软启动
    const uint8_t soft_start_enabled =
        (motor_data.config[port][0] == 0x00) && ((motor_data.soft_start_stop_switch.value >> port) & 0x01);

    if (!soft_start_enabled) {
        setMotorOutput(port, throttle);
        return;
    }

    // 软启动处理
    const uint32_t now       = micros();
    const uint32_t prev_time = my_timestamp_prev[port];
    float Ts                 = (now - prev_time) * 1e-6f;

    setpoint[port] = throttle;  // 更新目标值

    // 检测状态变化
    if (last_setpoint[port] != throttle) {
        if (abs(last_setpoint[port]) <= 20) {
            is_bootup[port] = 1;
        } else if (abs(throttle) <= 20) {
            is_bootup[port] = 2;
        } else if ((setpoint[port] > 0 && last_setpoint[port] < 0) || (setpoint[port] < 0 && last_setpoint[port] > 0)) {
            is_bootup[port] = 3;
        }
        output[port]        = throttle;
        last_setpoint[port] = throttle;
    }

    // 无软启动状态
    if (!is_bootup[port]) {
        setMotorOutput(port, throttle);
        output_prev[port]       = output[port];
        my_timestamp_prev[port] = now;
        return;
    }

    // 软启动状态处理
    if (Ts < 0.1f) return;  // 时间间隔不足，跳过

    // 加速度限制处理
    if (normal_motor_pid_velocity_output_ramp > 0) {
        const float output_rate = (setpoint[port] - output_prev[port]) / Ts;
        const float ramp_limit  = normal_motor_pid_velocity_output_ramp;
        const float ramp_temp   = -ramp_limit;

        if (output_rate > ramp_limit) {
            output[port] = output_prev[port] + ramp_limit * Ts;
        } else if (output_rate < ramp_temp) {
            output[port] = output_prev[port] + ramp_temp * Ts;
        } else {
            output[port]    = setpoint[port];
            is_bootup[port] = 0;  // 完成软启动
        }
    }

    // 应用输出并更新状态
    setMotorOutput(port, output[port]);
    output_prev[port]       = output[port];
    my_timestamp_prev[port] = now;
}

void InitMotorPwm(void)
{
// initial hardware pwm
#if MPWM_HIGHFREQUENCY
    TIM16->PSC = 35 - 1;
    TIM16->ARR = 255 - 1;
#else
    TIM16->PSC = 250 - 1;
    TIM16->ARR = 255 - 1;
#endif

    // reset motor throttle
    SetMotorThrottle(PWM_M1, 0);

    // start pwm generating
    HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
}

#include "../myfiles/motorbdc.h"

#include "tim.h"

#include "sys_utils.h"

#include <stdlib.h>

#define M1R_H LL_GPIO_SetOutputPin(MCU_DIR_M1R_GPIO_Port, MCU_DIR_M1R_Pin)
#define M1R_L LL_GPIO_ResetOutputPin(MCU_DIR_M1R_GPIO_Port, MCU_DIR_M1R_Pin)
#define M1F_H LL_GPIO_SetOutputPin(MCU_DIR_M1F_GPIO_Port, MCU_DIR_M1F_Pin)
#define M1F_L LL_GPIO_ResetOutputPin(MCU_DIR_M1F_GPIO_Port, MCU_DIR_M1F_Pin)

#define M2R_H LL_GPIO_SetOutputPin(MCU_DIR_M2R_GPIO_Port, MCU_DIR_M2R_Pin)
#define M2R_L LL_GPIO_ResetOutputPin(MCU_DIR_M2R_GPIO_Port, MCU_DIR_M2R_Pin)
#define M2F_H LL_GPIO_SetOutputPin(MCU_DIR_M2F_GPIO_Port, MCU_DIR_M2F_Pin)
#define M2F_L LL_GPIO_ResetOutputPin(MCU_DIR_M2F_GPIO_Port, MCU_DIR_M2F_Pin)

#define M3R_H LL_GPIO_SetOutputPin(MCU_DIR_M3R_GPIO_Port, MCU_DIR_M3R_Pin)
#define M3R_L LL_GPIO_ResetOutputPin(MCU_DIR_M3R_GPIO_Port, MCU_DIR_M3R_Pin)
#define M3F_H LL_GPIO_SetOutputPin(MCU_DIR_M3F_GPIO_Port, MCU_DIR_M3F_Pin)
#define M3F_L LL_GPIO_ResetOutputPin(MCU_DIR_M3F_GPIO_Port, MCU_DIR_M3F_Pin)

#define M4R_H LL_GPIO_SetOutputPin(MCU_DIR_M4R_GPIO_Port, MCU_DIR_M4R_Pin)
#define M4R_L LL_GPIO_ResetOutputPin(MCU_DIR_M4R_GPIO_Port, MCU_DIR_M4R_Pin)
#define M4F_H LL_GPIO_SetOutputPin(MCU_DIR_M4F_GPIO_Port, MCU_DIR_M4F_Pin)
#define M4F_L LL_GPIO_ResetOutputPin(MCU_DIR_M4F_GPIO_Port, MCU_DIR_M4F_Pin)

uint32_t normal_motor_pid_velocity_output_ramp = 100;
float normal_output_rate[4]                    = {0};
int16_t setpoint[4] = {0}, last_setpoint[4] = {0};
int16_t output[4] = {0}, output_prev[4] = {0};
uint32_t my_timestamp_now[4]  = {0};
uint32_t my_timestamp_prev[4] = {0};
float my_Ts[4]                = {0};
uint8_t is_bootup[4]          = {0};

void SetMotorDir(uint8_t port, uint8_t dir)
{
    switch (port) {
        case PWM_M1: {
            if (dir) {
                M1F_L;
                M1R_H;
            } else {
                M1R_L;
                M1F_H;
            }
            break;
        }
        case PWM_M2: {
            if (dir) {
                M2F_L;
                M2R_H;
            } else {
                M2R_L;
                M2F_H;
            }
            break;
        }
        case PWM_M3: {
            if (dir) {
                M3F_L;
                M3R_H;
            } else {
                M3R_L;
                M3F_H;
            }
            break;
        }
        case PWM_M4: {
            if (dir) {
                M4F_L;
                M4R_H;
            } else {
                M4R_L;
                M4F_H;
            }
            break;
        }
    }
}

void SetMotorPwm(uint8_t port, uint8_t value)
{
    switch (port) {
        case PWM_M1: {
            TIM1->CCR2 = value;
            break;
        }
        case PWM_M2: {
            TIM1->CCR1 = value;
            break;
        }
        case PWM_M3: {
            TIM1->CCR4 = value;
            break;
        }
        case PWM_M4: {
            TIM1->CCR3 = value;
            break;
        }
    }
}

static inline void setMotorOutput(uint8_t port, int16_t output_val)
{
    if (output_val > 0) {
        SetMotorDir(port, MDIR_REV);
        // SetMotorDir(port, MDIR_FFW);
        // 防止溢出的安全计算
        uint16_t pwm_val = ((uint32_t)output_val * 255u) / 127u;
        SetMotorPwm(port, pwm_val > 255 ? 255 : pwm_val);
    } else {
        SetMotorDir(port, MDIR_FFW);
        //    	SetMotorDir(port, MDIR_REV);
        uint16_t abs_val = (uint16_t)(-output_val);
        uint16_t pwm_val = ((uint32_t)abs_val * 255u) / 127u;
        SetMotorPwm(port, pwm_val > 255 ? 255 : pwm_val);
    }
}

void SetMotorThrottle(uint8_t port, int8_t throttle)
{
    // 检查是否启用软启动
    const uint8_t soft_start_enabled =
        (motor_data.config[port][0] != 0x01) && ((motor_data.soft_start_stop_switch.value >> port) & 0x01);

    if (!soft_start_enabled) {
        setMotorOutput(port, throttle);
        return;
    }

    motor_data.is_not_finish_flag |= (1 << port);

    // 软启动处理
    const uint32_t now       = micros();
    const uint32_t prev_time = my_timestamp_prev[port];
    float Ts                 = (now - prev_time) * 1e-6f;

    setpoint[port] = throttle;  // 更新目标值

    // 检测状态变化
    if (last_setpoint[port] != throttle) {
        if (abs(last_setpoint[port]) <= 20) {
            is_bootup[port] = 1;  // 从静止状态启动
        } else if (abs(throttle) <= 20) {
            is_bootup[port] = 2;  // 运动到静止状态
        } else if ((setpoint[port] > 0 && last_setpoint[port] < 0) || (setpoint[port] < 0 && last_setpoint[port] > 0)) {
            is_bootup[port] = 3;  // 方向反转
        }
        output[port]        = throttle;  // 更新本次输出值
        last_setpoint[port] = throttle;  // 更新上次输出值为本次输出值
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
            motor_data.is_not_finish_flag &= ~(1 << port);
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
    TIM1->PSC = 32 - 1;
    TIM1->ARR = 255 - 1;
#else
    TIM1->PSC = 188 - 1;
    TIM1->ARR = 255 - 1;
#endif

    // reset motor throttle
    SetMotorThrottle(PWM_M1, 0);
    SetMotorThrottle(PWM_M2, 0);
    SetMotorThrottle(PWM_M3, 0);
    SetMotorThrottle(PWM_M4, 0);

    // start pwm generating
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}

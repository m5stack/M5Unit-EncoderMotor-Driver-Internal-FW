#ifndef __PID_H_
#define __PID_H_

#include "main.h"

typedef struct {
    uint8_t* kp;  // 比例系数Proportional，范围0~255
    uint8_t* ki;  // 积分系数Integral，范围0~255
    uint8_t* kd;  // 微分系数Derivative，范围0~255

    int32_t ek;   // 当前误差
    int32_t ek1;  // 前一次误差 e(k-1)
    int32_t ek2;  // 再前一次误差 e(k-2)
} PID_IncType_t;

typedef struct {
    uint8_t* kp;  // 比例系数Proportional，范围0~255
    uint8_t* ki;  // 积分系数Integral，范围0~255
    uint8_t* kd;  // 微分系数Derivative，范围0~255

    int32_t ek1;
    int32_t i_sum;  // 误差累加
    int32_t i_max;
    int32_t i_min;
} pid_pos_t;

extern int32_t PIDInc(PID_IncType_t* pid, int32_t aims, int32_t input);
extern int32_t PIDPos(pid_pos_t* pid, int32_t aims, int32_t input);

#define MAX(in, max)            (((in) > (max)) ? (in) : (max))
#define MIN(in, min)            (((in) < (min)) ? (in) : (min))
#define CONSTRAIN(in, min, max) MAX(min, MIN(in, max))
#define INTEGRAL_TIME           10

#endif

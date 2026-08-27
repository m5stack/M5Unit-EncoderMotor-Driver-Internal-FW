#include "pid.h"

static int64_t constrain_i64(int64_t value, int64_t min_value, int64_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

int32_t PIDInc(PID_IncType_t* pid, int32_t aims, int32_t input)
{
    int64_t out_inc;  // 增量

    pid->ek  = aims - input;
    out_inc  = *pid->kp * (pid->ek - pid->ek1) + *pid->ki * pid->ek + *pid->kd * (pid->ek - 2 * pid->ek1 + pid->ek2);
    pid->ek2 = pid->ek1;
    pid->ek1 = pid->ek;
    return CONSTRAIN(out_inc, INT32_MIN, INT32_MAX);
}

int32_t PIDPos(pid_pos_t* pid, int32_t aims, int32_t input)
{
    int64_t error = (int64_t)aims - (int64_t)input;
    int64_t i_sum = (int64_t)pid->i_sum + error * (int64_t)(*pid->ki);
    pid->i_sum    = (int32_t)constrain_i64(i_sum, pid->i_min, pid->i_max);

    int64_t out_inc = (int64_t)(*pid->kp) * error + pid->i_sum + (int64_t)(*pid->kd) * (error - (int64_t)pid->ek1);

    pid->ek1 = (int32_t)constrain_i64(error, INT32_MIN, INT32_MAX);
    return (int32_t)constrain_i64(out_inc, INT32_MIN, INT32_MAX);
}

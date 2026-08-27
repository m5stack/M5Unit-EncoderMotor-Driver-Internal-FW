#include "../myfiles/pid.h"

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
    int64_t out_inc;
    int32_t error = aims - input;
    pid->i_sum    = pid->i_sum + error * (*pid->ki);
    pid->i_sum    = CONSTRAIN(pid->i_sum, pid->i_min, pid->i_max);
    out_inc       = *pid->kp * error + pid->i_sum + *pid->kd * (error - pid->ek1);
    pid->ek1      = error;
    return CONSTRAIN(out_inc, INT32_MIN, INT32_MAX);
}

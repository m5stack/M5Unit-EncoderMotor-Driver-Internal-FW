#ifndef __MYMOTORBDC_H
#define __MYMOTORBDC_H

#include "main.h"

#define MPWM_HIGHFREQUENCY 1

#define PWM_M1 0
#define PWM_M2 1
#define PWM_M3 2
#define PWM_M4 3

#define MDIR_FFW 0
#define MDIR_REV 1

extern int8_t speed[4];

void InitMotorPwm(void);
void SetMotorThrottle(uint8_t port, int8_t throttle);
#endif

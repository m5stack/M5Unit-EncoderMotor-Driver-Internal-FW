#ifndef __MYMOTORENCODER_H
#define __MYMOTORENCODER_H
#include "main.h"

#define ENC_M1 0
#define ENC_M2 1
#define ENC_M3 2
#define ENC_M4 3

// #define INVERT_ENC_M1 0
// #define INVERT_ENC_M2 0
// #define INVERT_ENC_M3 0
// #define INVERT_ENC_M4 0

typedef struct {
    union {
        struct {
            int32_t encoder_raw_0 : 8;
            int32_t encoder_raw_1 : 8;
            int32_t encoder_raw_2 : 8;
            int32_t encoder_raw_3 : 8;
        } flags;
        int32_t value;  // 整体访问接口
    } encoder_raw;
} encoder_raw_t;

extern volatile encoder_raw_t encoder_raw[4];

void InitMotorEncoder(void);
int32_t GetEncoderPos(uint8_t port);
void SetEncoderPos(uint8_t port, int32_t pos);
void ResetEncoderPos(uint8_t port);
void InterruptMotorEncoder(void);

#endif

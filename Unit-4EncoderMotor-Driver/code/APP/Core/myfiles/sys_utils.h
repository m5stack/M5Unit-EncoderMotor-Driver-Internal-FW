#ifndef __SYS_UTILS_H__
#define __SYS_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "pid.h"
#include "ws2812.h"

#define MOTOR_M1 0
#define MOTOR_M2 1
#define MOTOR_M3 2
#define MOTOR_M4 3

#define CONFIG_QUEUE_SIZE 8

#define MOTOR_MODE_A 0
#define MOTOR_MODE_B 1

typedef enum {
    MOTOR_CONFIG_MOTOR_MODE = 0,
    MOTOR_CONFIG_MOTOR_POS_P,
    MOTOR_CONFIG_MOTOR_POS_I,
    MOTOR_CONFIG_MOTOR_POS_D,
    MOTOR_CONFIG_MOTOR_POS_POINT_BYTE0,
    MOTOR_CONFIG_MOTOR_POS_POINT_BYTE1,
    MOTOR_CONFIG_MOTOR_POS_POINT_BYTE2,
    MOTOR_CONFIG_MOTOR_POS_POINT_BYTE3,
    MOTOR_CONFIG_MOTOR_POS_MAX_SPEED,
    MOTOR_CONFIG_MOTOR_SPEED_P,
    MOTOR_CONFIG_MOTOR_SPEED_I,
    MOTOR_CONFIG_MOTOR_SPEED_D,
    MOTOR_CONFIG_MOTOR_SPEED_POINT,
} MOTOR_CONFIG_INDEX_T;

typedef struct {
    int8_t motor_throttle[4];
    int8_t speed[4];
    uint8_t config[4][13];
    union {
        struct {
            uint8_t motor0 : 1;
            uint8_t motor1 : 1;
            uint8_t motor2 : 1;
            uint8_t motor3 : 1;
            uint8_t rsvd : 4;
        } flags;
        uint8_t value;  // 整体访问接口
    } soft_start_stop_switch;
    union {
        struct {
            uint8_t motor0 : 1;
            uint8_t motor1 : 1;
            uint8_t motor2 : 1;
            uint8_t motor3 : 1;
            uint8_t rsvd : 4;
        } flags;
        uint8_t is_not_finish_flag;
    };
    union {
        struct {
            uint8_t motor0 : 1;
            uint8_t motor1 : 1;
            uint8_t motor2 : 1;
            uint8_t motor3 : 1;
            uint8_t rsvd : 4;
        } flags;
        uint8_t value;  // 整体访问接口
    } nsleep;
    uint8_t encoder_ab_mode;
} motor_data_t;

extern volatile motor_data_t motor_data;
extern volatile pid_pos_t pid_speed[4];
extern volatile pid_pos_t pid_pos[4];

void init_motor(void);
long map(long x, long in_min, long in_max, long out_min, long out_max);
uint8_t dequeue_config_request(rgb_data_t *rgb);
void enqueue_config_request(rgb_data_t rgb);
uint8_t config_queue_has_data(void);

#ifdef __cplusplus
}
#endif

#endif

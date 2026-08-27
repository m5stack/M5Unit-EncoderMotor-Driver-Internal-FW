#include "sys_utils.h"
#include "mymotorencoder.h"

volatile motor_data_t motor_data = {0};
volatile pid_pos_t pid_speed[4]  = {0};
volatile pid_pos_t pid_pos[4]    = {0};

volatile rgb_data_t config_queue[CONFIG_QUEUE_SIZE];
static volatile uint8_t queue_head = 0, queue_tail = 0;

void init_motor(void)
{
    LL_GPIO_SetOutputPin(DRV_NSLEEP_GPIO_Port, DRV_NSLEEP_Pin);
    motor_data.motor_sleep                  = 1;
    encoder_raw[MOTOR_M1].encoder_raw.value = 0;
    motor_data.motor_throttle[MOTOR_M1]     = 0;

    motor_data.config[MOTOR_M1][1] = 1;    // position-p
    motor_data.config[MOTOR_M1][2] = 1;    // position-i
    motor_data.config[MOTOR_M1][3] = 17;   // position-d
    motor_data.config[MOTOR_M1][8] = 127;  // position max speed

    motor_data.config[MOTOR_M1][9]  = 10;  // speed-p
    motor_data.config[MOTOR_M1][10] = 1;   // speed-i
    motor_data.config[MOTOR_M1][11] = 15;  // speed-d

    pid_pos[MOTOR_M1].kp = (uint8_t *)&motor_data.config[MOTOR_M1][1];
    pid_pos[MOTOR_M1].ki = (uint8_t *)&motor_data.config[MOTOR_M1][2];
    pid_pos[MOTOR_M1].kd = (uint8_t *)&motor_data.config[MOTOR_M1][3];

    pid_speed[MOTOR_M1].kp = (uint8_t *)&motor_data.config[MOTOR_M1][9];
    pid_speed[MOTOR_M1].ki = (uint8_t *)&motor_data.config[MOTOR_M1][10];
    pid_speed[MOTOR_M1].kd = (uint8_t *)&motor_data.config[MOTOR_M1][11];

    pid_pos[MOTOR_M1].i_max = 7;
    pid_pos[MOTOR_M1].i_min = -7;

    pid_speed[MOTOR_M1].i_min = -10;
    pid_speed[MOTOR_M1].i_max = 10;
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    long result;

    result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    if (result < out_min)
        result = out_min;
    else if (result > out_max)
        result = out_max;

    return result;
}

uint8_t dequeue_config_request(rgb_data_t *rgb)
{
    __disable_irq();

    uint8_t success = 0;
    if (queue_head != queue_tail) {  //
        *rgb       = config_queue[queue_tail];
        queue_tail = (queue_tail + 1) % CONFIG_QUEUE_SIZE;
        success    = 1;
    }

    __enable_irq();

    return success;  // 返回操作状态
}

void enqueue_config_request(rgb_data_t rgb)
{
    config_queue[queue_head] = rgb;
    queue_head               = (queue_head + 1) % CONFIG_QUEUE_SIZE;
}

uint8_t config_queue_has_data(void)
{
    return (queue_head != queue_tail);
}

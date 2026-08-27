# Unit 4EncoderMotor 系统架构流程图

## 系统概述
Unit 4EncoderMotor是一款四通道编码直流电机驱动单元，采用STM32F030C8T6为控制核心，通过I2C进行通信，支持PWM、速度和位置三种控制模式。

## 主要组件架构

### 1. 系统初始化流程 (main.c)
```
main() 入口
├── iap_set() - IAP设置
├── HAL_Init() - HAL库初始化
├── SystemClock_Config() - 系统时钟配置
├── 外设初始化
│   ├── MX_GPIO_Init() - GPIO初始化
│   ├── MX_DMA_Init() - DMA初始化
│   ├── MX_ADC_Init() - ADC初始化
│   ├── MX_TIM1_Init() - 定时器1初始化(PWM)
│   ├── MX_I2C2_Init() - I2C2初始化
│   └── MX_IWDG_Init() - 看门狗初始化
├── 用户模块初始化
│   ├── init_flash_data() - Flash数据初始化
│   ├── user_gpio_init() - 用户GPIO初始化
│   ├── sk6812_init() - RGB LED初始化
│   ├── InitMotorPwm() - 电机PWM初始化
│   ├── InitMotorEncoder() - 编码器初始化
│   ├── init_motor() - 电机数据初始化
│   ├── init_pt_task() - ProtoThreads任务初始化
│   └── user_i2c_init() - I2C从机初始化
└── 主循环: main_scheduler(&main_pt)
```

### 2. ProtoThreads任务调度系统 (pt_task.c)
```
main_scheduler() 主调度器
├── stm_feed_wdt_handle() - 看门狗喂狗 (10ms周期)
├── pos_loop_handle() - 位置控制循环 (1ms周期)
├── speed_loop_handle() - 速度控制循环 (20ms周期)
├── motor_pwm_update_handle() - PWM输出更新
├── motor_soft_start_and_stop_handle() - 软启停处理
├── rgb_update_timeout_handle() - RGB超时处理
├── rgb_update_handle() - RGB更新处理
├── adc_to_voltage_handle() - ADC电压转换 (50μs周期)
├── i2c_addr_config_setter() - I2C地址配置
├── flash_config_setter() - Flash配置保存
├── i2c_slave_timeout_handle() - I2C从机超时处理
└── jump_to_bootloader_setter() - 跳转到Bootloader
```

### 3. I2C通信系统 (i2c_ex.c)
```
I2C从机通信
├── I2C2_IRQHandler() - I2C中断处理
│   ├── 地址匹配检测
│   ├── 读写方向判断
│   ├── 数据接收/发送
│   └── 错误处理
├── Slave_Complete_Callback() - 数据处理回调
│   ├── 电机控制数据 (0x20-0x23)
│   ├── 编码器数据 (0x30-0x3F)
│   ├── 电机配置数据 (0x50-0x8C)
│   ├── 系统设置 (0x90-0x92)
│   ├── RGB设置 (0xA0-0xA4)
│   └── 系统信息查询
└── 数据寄存器映射
    ├── 写入寄存器: 电机油门、编码器清零、配置参数
    └── 读取寄存器: 速度、位置、电流、电压、状态
```

### 4. 电机控制系统 (motorbdc.c)
```
电机控制
├── SetMotorThrottle() - 设置电机油门
│   ├── 油门值范围: -127 ~ +127
│   ├── 方向控制: SetMotorDir()
│   └── PWM输出: SetMotorPwm()
├── 四路电机独立控制
│   ├── Motor 1: TIM1_CH2 (PWM) + GPIO方向控制
│   ├── Motor 2: TIM1_CH1 (PWM) + GPIO方向控制
│   ├── Motor 3: TIM1_CH4 (PWM) + GPIO方向控制
│   └── Motor 4: TIM1_CH3 (PWM) + GPIO方向控制
└── H桥驱动器控制 (DRV8837)
    ├── 正转: xF_L + xR_H
    └── 反转: xF_H + xR_L
```

### 5. 编码器系统 (motorencoder.c)
```
编码器处理
├── InterruptMotorEncoder() - 编码器中断处理
│   ├── AB相正交解码
│   ├── 方向判断
│   └── 脉冲计数
├── 四路编码器输入
│   ├── Motor 1: PA6(A) + PA7(B)
│   ├── Motor 2: PA4(A) + PA5(B)
│   ├── Motor 3: PB7(A) + PB6(B)
│   └── Motor 4: PB9(A) + PB8(B)
├── 编码器模式切换
│   ├── 模式0: A相中断触发
│   └── 模式1: B相中断触发
└── 位置数据存储: encoder_raw[4]
```

### 6. PID控制算法 (pid.c)
```
PID控制器
├── PIDPos() - 位置式PID
│   ├── 比例项: Kp * error
│   ├── 积分项: Ki * ∑error (带限幅)
│   └── 微分项: Kd * (error - last_error)
├── PIDInc() - 增量式PID
│   ├── 增量计算
│   └── 输出限幅
└── 控制应用
    ├── 速度控制: pid_speed[4]
    └── 位置控制: pid_pos[4]
```

### 7. 控制模式
```
三种控制模式
├── PWM模式 (0x00)
│   └── 直接PWM输出，无反馈控制
├── 位置模式 (0x01)
│   ├── 目标位置设定
│   ├── 编码器位置反馈
│   ├── PID位置控制
│   └── 最大速度限制
└── 速度模式 (0x02)
    ├── 目标速度设定
    ├── 编码器速度计算
    └── PID速度控制
```

### 8. 电源监控系统 (power.c + ADC)
```
电源监控
├── ADC采样 (DMA方式)
│   ├── 通道0: INA199电流检测正端
│   ├── 通道1: 电源电压检测 (分压)
│   ├── 通道2: INA199电流检测负端
│   └── 通道3: 内部基准电压
├── 数据处理
│   ├── 中值滤波 + 移动平均
│   ├── 差分电压计算
│   └── 电流计算 (0.5Ω分流电阻)
└── 实时监控
    ├── 输入电压 (mV)
    └── 输入电流 (mA)
```

### 9. 辅助功能
```
辅助系统
├── RGB LED控制 (ws2812.c)
│   ├── SK6812 RGB LED
│   ├── 颜色设置
│   └── 亮度控制
├── Flash存储 (flash.c)
│   ├── 配置参数保存
│   ├── I2C地址存储
│   └── 系统设置保存
└── 系统工具 (sys_utils.c)
    ├── 数据结构定义
    ├── 工具函数
    └── 配置队列管理
```

## 数据流向

### 输入数据流
```
I2C主机 → I2C从机接收 → 数据解析 → 参数更新 → 控制执行
```

### 控制数据流
```
目标值设定 → PID控制器 → PWM输出 → 电机驱动 → 编码器反馈 → 位置/速度计算
```

### 监控数据流
```
ADC采样 → 数据滤波 → 电压/电流计算 → I2C数据准备 → 主机查询响应
```

## 关键特性

1. **多任务协作**: 基于ProtoThreads的协作式多任务
2. **实时控制**: 1ms位置控制，20ms速度控制
3. **通信协议**: 完整的I2C寄存器映射
4. **安全保护**: 看门狗、超时检测、参数限幅
5. **灵活配置**: 三种控制模式，参数可调
6. **状态监控**: 实时电压电流监测
7. **用户界面**: RGB LED状态指示

## 系统优势

- **高精度控制**: 编码器反馈 + PID算法
- **多电机独立**: 四路电机独立控制
- **实时响应**: 协作式任务调度
- **通信可靠**: I2C协议 + 超时保护
- **功能完整**: 控制 + 监控 + 配置一体化


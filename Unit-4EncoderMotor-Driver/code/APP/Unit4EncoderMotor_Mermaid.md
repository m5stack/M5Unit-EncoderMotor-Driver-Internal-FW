# Unit 4EncoderMotor Mermaid 流程图

查看方法：VsCode打开当前文件，右键选择“重新打开编辑器的方式”--> "MarkDown Editor"，即可查看。

## 系统总体架构图

```mermaid
graph TB
    subgraph "Unit 4EncoderMotor 系统架构"
        A[STM32F030C8T6<br/>控制核心] --> B[I2C从机通信]
        A --> C[四路电机控制]
        A --> D[编码器处理]
        A --> E[电源监控]
        A --> F[RGB LED指示]
      
        B --> G[主控板<br/>I2C主机]
        C --> H[DRV8837<br/>H桥驱动器]
        H --> I[直流电机 x4]
        D --> J[AB相编码器 x4]
        E --> K[INA199<br/>电流监控]
        F --> L[WS2812<br/>RGB LED]
    end
```

## 系统状态机图

```mermaid
stateDiagram-v2
    [*] --> PowerOn : 系统上电
    PowerOn --> Initializing : 开始初始化

    state Initializing {
        [*] --> HardwareInit : 硬件初始化
        HardwareInit --> PeripheralInit : 外设初始化
        PeripheralInit --> UserModuleInit : 用户模块初始化
        UserModuleInit --> [*] : 初始化完成
    }    

    Initializing --> Running : 初始化完成

    state Running {
        [*] --> TaskScheduling : 任务调度
        TaskScheduling --> WatchdogFeed : 喂狗
        WatchdogFeed --> PositionControl : 位置控制
        PositionControl --> SpeedControl : 速度控制
        SpeedControl --> PWMUpdate : PWM更新
        PWMUpdate --> ADCProcess : ADC采样
        ADCProcess --> I2CHandle : I2C处理
        I2CHandle --> RGBUpdate : RGB更新
        RGBUpdate --> FlashConfig : Flash操作
        FlashConfig --> TaskScheduling : 下次循环
        TaskScheduling --> I2CComm : I2C通信请求
        I2CComm --> TaskScheduling : 通信完成
        TaskScheduling --> BootloaderJump : 跳转请求
    }

    Running --> BootloaderJump : 跳转至Bootloader
    BootloaderJump --> [*] : 系统重启

    Running --> Error : 严重错误
    Error --> [*] : 系统复位
```

## I2C通信系统流程图

```mermaid
flowchart TD
    I2CStart[I2C通信开始] --> IRQ[I2C2_IRQHandler<br/>I2C中断处理]
    IRQ --> AddrMatch{地址匹配检测}
    AddrMatch -->|匹配| Direction{读写方向判断}
    AddrMatch -->|不匹配| End[结束]
  
    Direction -->|写入| RxMode[接收模式<br/>启用RX中断]
    Direction -->|读取| TxMode[发送模式<br/>启用TX中断]
  
    RxMode --> RxData[接收数据]
    TxMode --> PrepareData[准备发送数据<br/>Slave_Complete_Callback]
    PrepareData --> TxData[发送数据]
  
    RxData --> Stop{STOP信号}
    TxData --> NACK{NACK信号}
  
    Stop -->|是| Callback[Slave_Complete_Callback<br/>数据处理回调]
    NACK -->|是| End
  
    Callback --> ParseData{数据解析}
    ParseData -->|0x20-0x23| MotorControl[电机控制数据]
    ParseData -->|0x30-0x3F| EncoderData[编码器数据]
    ParseData -->|0x50-0x8C| MotorConfig[电机配置数据]
    ParseData -->|0x90-0x92| SystemSet[系统设置]
    ParseData -->|0xA0-0xA4| RGBSet[RGB设置]
    ParseData -->|其他| SystemInfo[系统信息查询]
  
    MotorControl --> End
    EncoderData --> End
    MotorConfig --> End
    SystemSet --> End
    RGBSet --> End
    SystemInfo --> End
```

## 电机控制系统流程图

```mermaid
flowchart TD
    MotorStart[电机控制开始] --> SetThrottle[SetMotorThrottle<br/>设置电机油门]
    SetThrottle --> CheckRange{油门值检查<br/>-127 ~ +127}
    CheckRange -->|超范围| Limit[限幅处理]
    CheckRange -->|正常| Direction{方向判断}
    Limit --> Direction
  
    Direction -->|正值| Forward[正转<br/>SetMotorDir FORWARD]
    Direction -->|负值| Reverse[反转<br/>SetMotorDir REVERSE]
    Direction -->|零值| Stop[停止<br/>PWM = 0]
  
    Forward --> PWMForward[SetMotorPwm<br/>正向PWM输出]
    Reverse --> PWMReverse[SetMotorPwm<br/>反向PWM输出]
  
    PWMForward --> HBridge[H桥驱动器控制<br/>DRV8837]
    PWMReverse --> HBridge
    Stop --> HBridge
  
    HBridge --> Motor1[Motor 1<br/>TIM1_CH2]
    HBridge --> Motor2[Motor 2<br/>TIM1_CH1]
    HBridge --> Motor3[Motor 3<br/>TIM1_CH4]
    HBridge --> Motor4[Motor 4<br/>TIM1_CH3]
  
    Motor1 --> GPIO1[GPIO方向控制<br/>M1F/M1R]
    Motor2 --> GPIO2[GPIO方向控制<br/>M2F/M2R]
    Motor3 --> GPIO3[GPIO方向控制<br/>M3F/M3R]
    Motor4 --> GPIO4[GPIO方向控制<br/>M4F/M4R]
```

## 编码器处理系统流程图

```mermaid
flowchart TD
    EncStart[中断触发] --> EncIRQ[中断处理]
    EncIRQ --> ModeCheck[模式检查]
    ModeCheck -->|模式0| ModeA[A相中断]
    ModeCheck -->|模式1| ModeB[B相中断]
    ModeA --> ReadGPIOA[读取GPIO]
    ModeB --> ReadGPIOB[读取GPIO]
    ReadGPIOA --> CheckMotorA[检查电机A]
    ReadGPIOB --> CheckMotorB[检查电机B]
    CheckMotorA -->|M1| DecodeM1A[M1解码A]
    CheckMotorA -->|M2| DecodeM2A[M2解码A]
    CheckMotorA -->|M3| DecodeM3A[M3解码A]
    CheckMotorA -->|M4| DecodeM4A[M4解码A]
    CheckMotorB -->|M1| DecodeM1B[M1解码B]
    CheckMotorB -->|M2| DecodeM2B[M2解码B]
    CheckMotorB -->|M3| DecodeM3B[M3解码B]
    CheckMotorB -->|M4| DecodeM4B[M4解码B]
    DecodeM1A --> QuadDecode1[方向判断1]
    DecodeM2A --> QuadDecode2[方向判断2]
    DecodeM3A --> QuadDecode3[方向判断3]
    DecodeM4A --> QuadDecode4[方向判断4]
    DecodeM1B --> QuadDecode1
    DecodeM2B --> QuadDecode2
    DecodeM3B --> QuadDecode3
    DecodeM4B --> QuadDecode4
    QuadDecode1 --> UpdatePos1[计数更新1]
    QuadDecode2 --> UpdatePos2[计数更新2]
    QuadDecode3 --> UpdatePos3[计数更新3]
    QuadDecode4 --> UpdatePos4[计数更新4]
    UpdatePos1 --> ClearFlag[清除中断]
    UpdatePos2 --> ClearFlag
    UpdatePos3 --> ClearFlag
    UpdatePos4 --> ClearFlag
```

## PID控制算法流程图

```mermaid
flowchart TD
    PIDStart[开始] --> PIDPos[位置式PID]
    PIDPos --> CalcError[误差计算]
    CalcError --> PropTerm[比例项P]
    CalcError --> IntTerm[积分项I]
    CalcError --> DiffTerm[微分项D]
    PropTerm --> IntLimit[积分限幅]
    IntTerm --> IntLimit
    DiffTerm --> SumPID[PID输出求和]
    IntLimit --> SumPID
    SumPID --> OutputLimit[输出限幅]
    OutputLimit --> UpdateHistory[更新历史]
    UpdateHistory --> PIDOutput[输出]
    PIDOutput --> Application[应用]
    Application -->|速度控制| SpeedPID[速度PID]
    Application -->|位置控制| PosPID[位置PID]
```

## 控制模式流程图

```mermaid
flowchart TD
    ControlStart[模式选择] --> ModeCheck[模式检查]
    ModeCheck -->|0x00| PWMMode[PWM模式]
    ModeCheck -->|0x01| PosMode[位置控制]
    ModeCheck -->|0x02| SpeedMode[速度控制]
    PWMMode --> DirectPWM[PWM输出]
    DirectPWM --> MotorOutput1[电机输出]
    PosMode --> SetTarget1[设定位置]
    SetTarget1 --> ReadPos[读取位置]
    ReadPos --> CalcPosError[误差计算]
    CalcPosError --> ReverseProtect1[换向保护]
    ReverseProtect1 -->|需保护| BrakeFirst1[刹车]
    ReverseProtect1 -->|正常| PosPIDCalc[位置PID]
    BrakeFirst1 --> MotorOutput2[电机输出]
    PosPIDCalc --> SpeedLimit[速度限制]
    SpeedLimit --> RampOutput1[斜坡]
    RampOutput1 --> MotorOutput2
    SpeedMode --> SetTarget2[设定速度]
    SetTarget2 --> CalcSpeed[当前速度]
    CalcSpeed --> CalcSpeedError[误差]
    CalcSpeedError --> ReverseProtect2[换向保护]
    ReverseProtect2 -->|需保护| BrakeFirst2[刹车]
    ReverseProtect2 -->|正常| SpeedPIDCalc[速度PID]
    BrakeFirst2 --> MotorOutput3[电机输出]
    SpeedPIDCalc --> ThrottleLimit[油门限幅]
    ThrottleLimit --> RampOutput2[斜坡]
    RampOutput2 --> MotorOutput3
```

## 电源监控系统流程图

```mermaid
flowchart TD
    ADCStart[ADC开始] --> ADCSample[采样]
    ADCSample --> Ch0[通道0 ADC_REF]
    ADCSample --> Ch1[通道1 ADC_VM]
    ADCSample --> Ch2[通道2 ADC_INA]
    ADCSample --> Ch3[通道3 ADC_VREFINT]
    Ch0 --> SafeRead0[安全读]
    Ch1 --> SafeRead1[安全读]
    Ch2 --> SafeRead2[安全读]
    Ch3 --> SafeRead3[安全读]
    SafeRead0 --> MedianFilter0[中值滤波]
    SafeRead1 --> MedianFilter1[中值滤波]
    SafeRead2 --> MedianFilter2[中值滤波]
    SafeRead3 --> MedianFilter3[中值滤波]
    MedianFilter0 --> MovingAvg0[移动平均]
    MedianFilter1 --> MovingAvg1[移动平均]
    MedianFilter3 --> MovingAvg3[移动平均]
    MovingAvg3 --> CalcVRef[基准电压]
    CalcVRef --> RefCheck[电压检查]
    RefCheck -->|不满足| ADCStart
    RefCheck -->|满足| InitCheck[初始化检查]
    InitCheck -->|未初始化| CalCount[校准计数]
    CalCount --> CountCheck[计数检查]
    CountCheck -->|未达标| ADCStart
    CountCheck -->|达标| SetInit[设置完成]
    InitCheck -->|已初始化| CalcVoltage[电压计算]
    SetInit --> CalcVoltage
    MovingAvg1 --> CalcVoltage
    CalcVoltage --> CalcDiff[差分电压]
    MovingAvg0 --> CalcDiff
    MedianFilter2 --> CalcDiff
    CalcDiff --> CalcCurrent[电流计算]
    CalcCurrent --> AdaptiveFilter[自适应滤波]
    AdaptiveFilter --> CurrentOutput[输出]
```

## 数据流向图

```mermaid
flowchart LR
    subgraph "输入数据流"
        I2CMaster[I2C主机] --> I2CSlave[I2C从机接收]
        I2CSlave --> DataParse[数据解析]
        DataParse --> ParamUpdate[参数更新]
        ParamUpdate --> ControlExec[控制执行]
    end
  
    subgraph "控制数据流"
        TargetSet[目标值设定] --> PIDController[PID控制器]
        PIDController --> PWMOut[PWM输出]
        PWMOut --> MotorDrive[电机驱动]
        MotorDrive --> EncFeedback[编码器反馈]
        EncFeedback --> SpeedPosCalc[位置/速度计算]
        SpeedPosCalc --> PIDController
    end
  
    subgraph "监控数据流"
        ADCSamp[ADC采样] --> DataFilter[数据滤波]
        DataFilter --> VoltCurrCalc[电压/电流计算]
        VoltCurrCalc --> I2CDataPrep[I2C数据准备]
        I2CDataPrep --> MasterQuery[主机查询响应]
    end
```


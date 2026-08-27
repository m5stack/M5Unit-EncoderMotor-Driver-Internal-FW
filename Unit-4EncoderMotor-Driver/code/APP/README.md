Unit 4Encoder Motor

Unit 4EncoderMotor是一款四通道编码直流电机驱动单元。其采用STM32F030C8T6为控制核心，通过I2C进行通信，每路电机均由DRV8837系列H桥驱动并配套AB脉冲编码器输入，实现转速与位置检测；集成INA199电源监控实时监测电压和电流状态。

2025.9.1
1. 提交0xF0开发固件和代码，功能包括四路电机三种模式控制、RGB灯亮度和颜色控制、I2C从机通信、ADC电压采集和INA电流监控、IAP升级。
2. 0xF0版本固件在firmware/P365-Unit_4_EncoderMotor-JYY-0xF0-0x08000000.bin, 流程图在Unit4EncoderMotor_Mermaid.md

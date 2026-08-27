#include "main.h"
#include "power.h"
#include "i2c.h"
#include "gpio.h"
#include "adc.h"
#include "tim.h"
#include "dma.h"
#include "ws2812.h"

extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_tim3_ch2;

typedef void (*pFunction)(void);
volatile uint32_t JumpAddress;
volatile pFunction JumpToBootloader;

uint8_t iwdg_is_enabled(void)
{
    // 检查状态寄存器是否配置完成
    if ((IWDG->SR & 0x03) != 0) {  // PVU或RVU为1，配置未完成
        return 0;
    }

    // 检查预分频和重装载值是否非默认值
    if ((IWDG->PR == 0) && (IWDG->RLR == 0xFFF)) {
        return 0;  // 未配置过IWDG
    }
    return 1;
}

static uint8_t Reset_AllPeriph(void)
{
    uint8_t status = HAL_OK;

    // 关闭I2C外设
    LL_I2C_DeInit(I2C2);
    LL_I2C_Disable(I2C2);
    i2c2_it_disable();

    // 关闭ADC外设
    if (HAL_ADC_GetState(&hadc1) != HAL_ADC_STATE_RESET) {
        HAL_ADC_Stop_DMA(&hadc1);
        HAL_ADC_DeInit(&hadc1);
    }

    // 关闭TIM外设
    HAL_TIM_Base_Stop(&htim3);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
    HAL_TIM_Base_Stop(&htim16);
    HAL_TIM_Base_DeInit(&htim3);
    HAL_TIM_Base_DeInit(&htim16);

    // 关闭DMA外设
    HAL_DMA_DeInit(&hdma_adc1);
    HAL_DMA_DeInit(&hdma_tim3_ch2);

    // 关闭GPIO外设时钟
    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOF_CLK_DISABLE();

    // 关闭外设时钟
    __HAL_RCC_ADC_CLK_DISABLE();
    __HAL_RCC_TIM3_CLK_DISABLE();
    __HAL_RCC_TIM16_CLK_DISABLE();
    __HAL_RCC_DMA1_CLK_DISABLE();
    __HAL_RCC_I2C2_CLK_DISABLE();

    // 清除系统配置和中断
    SysTick->CTRL = 0;
    SYSCFG->CFGR1 &= SYSCFG_CFGR1_MEM_MODE;
    NVIC->ICER[0] = 0xFFFFFFFF;
    NVIC->ICPR[0] = 0xFFFFFFFF;

    /* Set EXTICRx registers to reset value */
    EXTI->EXTICR[0] = 0;
    EXTI->EXTICR[1] = 0;
    EXTI->EXTICR[2] = 0;
    EXTI->EXTICR[3] = 0;

    /* Set CFGR2 register to reset value: clear SRAM parity error flag */
    SYSCFG->CFGR2 |= (uint32_t)SYSCFG_CFGR2_SRAM_PE;
    status |= HAL_RCC_DeInit();
    __HAL_SYSCFG_CLEAR_FLAG();

    // HAL_DeInit();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    __disable_irq();

    return status;
}

void Jump_Bootloader(void)
{
    uint32_t jump_bootloader_timeout = 0;

    if (((*(__IO uint32_t*)BOOTLOADER_ADDRESS) & 0x2FFE0000) == 0x20000000) {
        while (Reset_AllPeriph() != HAL_OK) {
            jump_bootloader_timeout++;
            if (jump_bootloader_timeout > 20) {
                jump_bootloader_timeout = 0;
                HAL_NVIC_SystemReset();
                HAL_Delay(1000);
            }
        }

        SCB->VTOR = BOOTLOADER_ADDRESS;
        /* Jump to user application */
        JumpAddress      = *(__IO uint32_t*)(BOOTLOADER_ADDRESS + 4);
        JumpToBootloader = (pFunction)JumpAddress;
        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO uint32_t*)BOOTLOADER_ADDRESS);
        JumpToBootloader();
    }
}

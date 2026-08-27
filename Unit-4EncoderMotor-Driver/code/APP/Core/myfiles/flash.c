/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    flash.c
 * @brief   This file provides code for the configuration
 *          of the flash instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "flash.h"

#include "sys_utils.h"
#include "i2c.h"
#include "pid.h"

uint8_t flash_data[FLASH_DATA_SIZE] = {0};

// 宏定义提高可维护性
#define MAX_RETRY_COUNT  20    // 单次操作最大重试次数
#define RESET_DELAY_MS   1000  // 复位前延时
#define FLASH_TIMEOUT_MS 100   // FLASH操作超时时间

static void system_reset(void);
static HAL_StatusTypeDef flash_recover_lock(void);

// 错误处理统一宏
#define CHECK_AND_RESET(op, err)  \
    do {                          \
        if ((err) != HAL_OK) {    \
            flash_recover_lock(); \
            system_reset();       \
        }                         \
    } while (0)

/*******************************************************************************
 * Function Name  : doseFlashHasPackedMessage
 * Description    : Does flash has packed messages
 * Input          : None
 * Output         :
 * Return         : ture/false
 *******************************************************************************/
bool doseFlashHasPackedMessage(void)
{
    uint16_t length;
    uint16_t getHead;

    /*Is head matched*/
    getHead = (uint16_t)(*(uint16_t *)(STM32F0xx_FLASH_PAGE63_STARTADDR));
    if (EEPPROM_PACKAGEHEAD != getHead) {
        return false;
    }

    /*Is length zero*/
    length = (*(uint16_t *)(STM32F0xx_FLASH_PAGE63_STARTADDR + 2));
    if (0 == length) {
        return false;
    }

    return true;
}
/*******************************************************************************
 * Function Name  : getValuablePackedMessageLengthofFlash
 * Description    : Get valuable packed message length of flash
 * Input          : None
 * Output         :
 * Return         : valuable length
 *******************************************************************************/
uint16_t getValuablePackedMessageLengthofFlash(void)
{
    uint16_t length;

    /*Is head matched*/
    if (EEPPROM_PACKAGEHEAD != (*(uint16_t *)(STM32F0xx_FLASH_PAGE63_STARTADDR))) {
        return 0;
    }

    /*Get length*/
    length = (uint16_t)(*(uint16_t *)(STM32F0xx_FLASH_PAGE63_STARTADDR + 2));

    return length;
}
/*******************************************************************************
 * Function Name  : readPackedMessageFromFlash
 * Description    : Read packed message form flash
 * Input          : buff:point to first location of received buffer.length:Maxmum length of reception
 * Output         :
 * Return         : reception length
 *******************************************************************************/
uint16_t readPackedMessageFromFlash(uint8_t *buff, uint16_t length)
{
    int i;
    uint16_t getLength;

    if (!doseFlashHasPackedMessage()) return 0;

    /*Get valuable length*/
    getLength = getValuablePackedMessageLengthofFlash();

    /*Read out message*/
    for (i = 0; i < MIN(getLength, length); i++) {
        buff[i] = *(uint8_t *)(STM32F0xx_FLASH_PAGE63_STARTADDR + 4 + i);
    }

    return MIN(getLength, length);
}
/*******************************************************************************
 * Function Name  : isItOddNumber
 * Description    : is input data an odd number?
 * Input          : number:input data
 * Output         :
 * Return         : true/false
 *******************************************************************************/
bool isItOddNumber(uint16_t number)
{
    if (0 != number % 2) {
        return true;
    }
    return false;
}
/*******************************************************************************
 * Function Name  : Flash_eeprom_WriteWithPacked
 * Description    : Write a group of datas to flash.
 * Input          : buff:pointer of first data, length: write length
 * Output         :
 * Return         : true/false
 *******************************************************************************/
bool writeMessageToFlash(uint8_t *buff, uint16_t length)
{
    uint16_t temp;
    int i;
    FLASH_EraseInitTypeDef My_Flash;

    /*Protection*/
    if ((length + 4) > STM32F0xx_PAGE_SIZE) {
        return false;
    }

    HAL_FLASH_Unlock();

    My_Flash.TypeErase   = FLASH_TYPEERASE_PAGES;             // 标明Flash执行页面只做擦除操作
    My_Flash.PageAddress = STM32F0xx_FLASH_PAGE63_STARTADDR;  // 声明要擦除的地址
    My_Flash.NbPages = 1;  // 说明要擦除的页数，此参数必须是Min_Data = 1和Max_Data =(最大页数-初始页的值)之间的值

    uint32_t PageError = 0;  // 设置PageError,如果出现错误这个变量会被设置为出错的FLASH地址
    HAL_FLASHEx_Erase(&My_Flash, &PageError);  // 调用擦除函数擦除

    // 对Flash进行烧写，FLASH_TYPEPROGRAM_HALFWORD
    // 声明操作的Flash地址的16位的，此外还有32位跟64位的操作，自行翻查HAL库的定义即可

    /*Write head*/
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, STM32F0xx_FLASH_PAGE63_STARTADDR, EEPPROM_PACKAGEHEAD);
    /*Write length*/
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, STM32F0xx_FLASH_PAGE63_STARTADDR + 2, length);

    /*Write datas*/
    for (i = 0; i < length / 2; i++) {
        temp = buff[2 * i] | (uint16_t)buff[2 * i + 1] << 8;
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, STM32F0xx_FLASH_PAGE63_STARTADDR + 4 + 2 * i, temp);
    }
    if (isItOddNumber(length))  // Write one more if length is odd number.
    {
        temp = (uint16_t)buff[length - 1];
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, STM32F0xx_FLASH_PAGE63_STARTADDR + 4 + (length - 1), temp);
    }

    /*Read out and check*/
    for (i = 0; i < length; i++) {
        if (*(uint8_t *)(STM32F0xx_FLASH_PAGE63_STARTADDR + 4 + i) != buff[i]) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    HAL_FLASH_Lock();
    return true;
}

void init_flash_data(void)
{
    uint32_t flash_write_timeout = 0;

    if (!(readPackedMessageFromFlash(flash_data, FLASH_DATA_SIZE))) {
        i2c_address[0] = I2C_ADDRESS;

        flash_data[0] = i2c_address[0];
        flash_data[1] = motor_data.encoder_ab_mode;
        flash_data[2] = motor_data.soft_start_stop_switch.value;

        // flash 写入失败，flash错误位置1
        while (!writeMessageToFlash(flash_data, FLASH_DATA_SIZE)) {
            flash_write_timeout++;
            if (flash_write_timeout > 20) {
                peri_error.flags.flash_err = 1;
                break;
            }
        }
    } else {
        i2c_address[0]                          = flash_data[0];
        motor_data.encoder_ab_mode              = flash_data[1];
        motor_data.soft_start_stop_switch.value = flash_data[2];
    }
}

uint8_t flash_data_write_back(void)
{
    uint32_t flash_write_timeout = 0;

    // if read flash ok
    if (readPackedMessageFromFlash(flash_data, FLASH_DATA_SIZE)) {
        flash_data[0] = i2c_address[0];
        flash_data[1] = motor_data.encoder_ab_mode;
        flash_data[2] = motor_data.soft_start_stop_switch.value;
        // flash写入失败，返回0，flash错误置1
        while (!writeMessageToFlash(flash_data, FLASH_DATA_SIZE)) {
            flash_write_timeout++;
            if (flash_write_timeout > 20) {
                peri_error.flags.flash_err = 1;
                flash_write_timeout        = 0;
                return 0;
            }
        }
        // write success, return 1
        return 1;
    } else {
        peri_error.flags.flash_err = 1;
        return 0;
    }
}

// 系统复位函数
static void system_reset(void)
{
    HAL_Delay(RESET_DELAY_MS);
    HAL_NVIC_SystemReset();
}

// FLASH资源恢复函数
static HAL_StatusTypeDef flash_recover_lock(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t start_tick      = HAL_GetTick();

    // 优先锁定OB
    while (HAL_FLASH_OB_Lock() != HAL_OK) {
        if (HAL_GetTick() - start_tick > FLASH_TIMEOUT_MS) {
            status = HAL_ERROR;
            break;
        }
    }

    // 然后锁定FLASH
    start_tick = HAL_GetTick();
    while (HAL_FLASH_Lock() != HAL_OK) {
        if (HAL_GetTick() - start_tick > FLASH_TIMEOUT_MS) {
            status = HAL_ERROR;
            break;
        }
    }

    return status;
}

// 带超时的FLASH操作封装
static HAL_StatusTypeDef flash_op_with_timeout(HAL_StatusTypeDef (*op)(void))
{
    uint32_t start_tick = HAL_GetTick();
    HAL_StatusTypeDef status;

    do {
        status = op();
        if (status == HAL_OK) break;
    } while (HAL_GetTick() - start_tick <= FLASH_TIMEOUT_MS);

    return status;
}

// 修改函数签名，接受带参数的函数指针和参数
static HAL_StatusTypeDef flash_op_with_timeout_and_para(HAL_StatusTypeDef (*op)(FLASH_OBProgramInitTypeDef *),
                                                        FLASH_OBProgramInitTypeDef *p_arg)
{
    uint32_t start_tick = HAL_GetTick();
    HAL_StatusTypeDef status;

    do {
        status = op(p_arg);  // 传递参数给函数指针
        if (status == HAL_OK) break;
    } while (HAL_GetTick() - start_tick <= FLASH_TIMEOUT_MS);

    return status;
}

// 优化后的选项字节编程函数
HAL_StatusTypeDef iwdg_ob_program(void)
{
    // FLASH_OBProgramInitTypeDef ob_init = {0};
    HAL_StatusTypeDef status = HAL_OK;

    // // 1. 获取当前OB配置
    // HAL_FLASHEx_OBGetConfig(&ob_init);

    // // 2. 检查是否需要修改IWDG配置 (STM32F0使用不同的位定义)
    // if ((ob_init.USERConfig & OB_IWDG_SW) &&
    //     (ob_init.USERConfig & OB_STDBY_NO_RST)) {
    //     return HAL_OK;  // 已经是期望配置，无需修改
    // }

    // // 3. 准备新配置 (STM32F0结构体字段)
    // ob_init.OptionType   = OPTIONBYTE_USER;
    // // STM32F0没有USERType字段，直接在USERConfig中配置
    // ob_init.USERConfig   = OB_IWDG_SW |           // IWDG软件启动
    //                       OB_STOP_NO_RST |       // STOP模式无复位
    //                       OB_STDBY_NO_RST;       // STANDBY模式无复位
    // ob_init.RDPLevel     = OB_RDP_LEVEL0;

    // // 4. 执行FLASH操作序列
    // do {
    //     // 4.1 解锁FLASH
    //     status = flash_op_with_timeout(HAL_FLASH_Unlock);
    //     CHECK_AND_RESET(HAL_FLASH_Unlock, status);

    //     // 4.2 解锁OB
    //     status = flash_op_with_timeout(HAL_FLASH_OB_Unlock);
    //     CHECK_AND_RESET(HAL_FLASH_OB_Unlock, status);

    //     // 4.3 编程OB
    //     status = flash_op_with_timeout_and_para(HAL_FLASHEx_OBProgram, &ob_init);
    //     CHECK_AND_RESET(HAL_FLASHEx_OBProgram, status);

    //     // 4.4 启动OB重载
    //     status = flash_op_with_timeout(HAL_FLASH_OB_Launch);
    //     CHECK_AND_RESET(HAL_FLASH_OB_Launch, status);

    // } while(0);

    // // 5. 无论成功与否都尝试恢复锁定
    // status = flash_recover_lock();

    return (status == HAL_OK) ? HAL_OK : HAL_ERROR;
}

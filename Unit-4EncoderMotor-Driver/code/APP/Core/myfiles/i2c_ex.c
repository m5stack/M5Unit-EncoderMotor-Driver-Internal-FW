/* Includes ------------------------------------------------------------------*/
#include "i2c_ex.h"
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "main.h"

#define UNUSED(X) (void)X /* To avoid gcc/g++ warnings */

__IO uint8_t aReceiveBuffer[I2C_RECEIVE_BUFFER_LEN];
__IO uint8_t tx_buffer[I2C_RECEIVE_BUFFER_LEN];
__IO uint16_t ubReceiveIndex           = 0;
volatile uint8_t i2c_addr              = 0;
volatile uint16_t tx_buffer_index      = 0;
volatile uint16_t tx_len               = 0;
volatile uint8_t i2c_slave_state       = I2C_SLAVE_STATE_IDLE;
volatile uint32_t i2c_ex_timeout_start = 0;
static volatile uint8_t rx_overflow    = 0;

void set_i2c_slave_address(uint8_t addr)
{
    i2c_addr = (addr << 1);
}

__weak void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(rx_data);
    UNUSED(len);
}

void i2c2_it_enable(void)
{
    LL_I2C_Enable(I2C2);

    /* (6) Enable I2C2 address match/error interrupts:
     *  - Enable Address Match Interrupt
     *  - Enable Not acknowledge received interrupt
     *  - Enable Error interrupts
     *  - Enable Stop interrupt
     */
    LL_I2C_EnableIT_ADDR(I2C2);
    LL_I2C_EnableIT_NACK(I2C2);
    LL_I2C_EnableIT_ERR(I2C2);
    LL_I2C_EnableIT_STOP(I2C2);
}

void i2c2_tx_rx_it_enable(void)
{
    LL_I2C_EnableIT_TX(I2C2);
    LL_I2C_EnableIT_RX(I2C2);
}

void i2c2_it_disable(void)
{
    LL_I2C_DisableIT_ADDR(I2C2);
    LL_I2C_DisableIT_TX(I2C2);
    LL_I2C_DisableIT_RX(I2C2);
    LL_I2C_DisableIT_NACK(I2C2);
    LL_I2C_DisableIT_ERR(I2C2);
    LL_I2C_DisableIT_STOP(I2C2);
}

void i2c2_reset(void)
{
    LL_I2C_DeInit(I2C2);
    LL_I2C_Disable(I2C2);
    i2c2_it_disable();
    ubReceiveIndex  = 0;
    rx_overflow     = 0;
    tx_buffer_index = 0;
    tx_len          = 0;
    user_i2c_init();
    i2c2_it_enable();
}

void i2c2_set_send_data(uint8_t *tx_ptr, uint16_t len)
{
    tx_buffer_index = 0;

    if (len == 0 || tx_ptr == NULL) {
        tx_len = 0;
        return;
    }

    if (len > I2C_RECEIVE_BUFFER_LEN) {
        len = I2C_RECEIVE_BUFFER_LEN;
    }
    memcpy((void *)tx_buffer, tx_ptr, len);
    tx_len = len;
}

void Slave_Reception_Callback(void)
{
    /* Read character in Receive Data register.
    RXNE flag is cleared by reading data in RXDR register */
    uint8_t byte = LL_I2C_ReceiveData8(I2C2);

    if (ubReceiveIndex < I2C_RECEIVE_BUFFER_LEN) {
        aReceiveBuffer[ubReceiveIndex] = byte;
        ubReceiveIndex++;
    } else {
        rx_overflow = 1;
    }
}

void Slave_Ready_To_Transmit_Callback(void)
{
    /* Send the Byte requested by the Master */
    if (tx_buffer_index < tx_len) {
        LL_I2C_TransmitData8(I2C2, tx_buffer[tx_buffer_index]);
        tx_buffer_index++;
    } else {
        LL_I2C_TransmitData8(I2C2, 0);
    }
}

static void i2c2_disable_tx_rx_it(void)
{
    LL_I2C_DisableIT_TX(I2C2);
    LL_I2C_DisableIT_RX(I2C2);
}

static void i2c2_finish_transfer(void)
{
    i2c2_disable_tx_rx_it();

    if (!LL_I2C_IsActiveFlag_TXE(I2C2)) {
        LL_I2C_ClearFlag_TXE(I2C2);
    }

    ubReceiveIndex  = 0;
    rx_overflow     = 0;
    tx_buffer_index = 0;
    i2c_slave_state = I2C_SLAVE_STATE_IDLE;
}

static void i2c2_enter_error_state(void)
{
    i2c2_disable_tx_rx_it();
    i2c2_set_send_data(NULL, 0);
    ubReceiveIndex       = 0;
    rx_overflow          = 0;
    i2c_ex_timeout_start = HAL_GetTick();
    i2c_slave_state      = I2C_SLAVE_STATE_ERROR;
}

void I2C2_IRQHandler(void)
{
    uint8_t error_detected = 0;

    if (LL_I2C_IsActiveFlag_BERR(I2C2)) {
        LL_I2C_ClearFlag_BERR(I2C2);
        error_detected = 1;
    }
    if (LL_I2C_IsActiveFlag_ARLO(I2C2)) {
        LL_I2C_ClearFlag_ARLO(I2C2);
        error_detected = 1;
    }
    if (LL_I2C_IsActiveFlag_OVR(I2C2)) {
        LL_I2C_ClearFlag_OVR(I2C2);
        error_detected = 1;
    }

    if (error_detected) {
        if (LL_I2C_IsActiveFlag_ADDR(I2C2)) {
            LL_I2C_ClearFlag_ADDR(I2C2);
        }
        if (LL_I2C_IsActiveFlag_STOP(I2C2)) {
            LL_I2C_ClearFlag_STOP(I2C2);
        }
        if (LL_I2C_IsActiveFlag_NACK(I2C2)) {
            LL_I2C_ClearFlag_NACK(I2C2);
        }
        i2c2_enter_error_state();
        return;
    }

    if (LL_I2C_IsActiveFlag_ADDR(I2C2)) {
        if (LL_I2C_GetAddressMatchCode(I2C2) == i2c_addr) {
            i2c_ex_timeout_start = HAL_GetTick();

            if (LL_I2C_GetTransferDirection(I2C2) == LL_I2C_DIRECTION_WRITE) {
                i2c2_disable_tx_rx_it();
                ubReceiveIndex  = 0;
                rx_overflow     = 0;
                tx_buffer_index = 0;

                LL_I2C_ClearFlag_ADDR(I2C2);
                LL_I2C_EnableIT_RX(I2C2);
                i2c_slave_state = I2C_SLAVE_STATE_READ;
            } else if (LL_I2C_GetTransferDirection(I2C2) == LL_I2C_DIRECTION_READ) {
                LL_I2C_DisableIT_RX(I2C2);
                tx_buffer_index = 0;

                LL_I2C_ClearFlag_ADDR(I2C2);

                if (ubReceiveIndex && !rx_overflow) {
                    Slave_Complete_Callback((uint8_t *)aReceiveBuffer, ubReceiveIndex);
                } else if (rx_overflow) {
                    i2c2_set_send_data(NULL, 0);
                }
                ubReceiveIndex = 0;
                rx_overflow    = 0;

                LL_I2C_EnableIT_TX(I2C2);
                if (LL_I2C_IsActiveFlag_TXIS(I2C2)) {
                    Slave_Ready_To_Transmit_Callback();
                }
                i2c_slave_state = I2C_SLAVE_STATE_WRITE;
            } else {
                LL_I2C_ClearFlag_ADDR(I2C2);
            }
        } else {
            LL_I2C_ClearFlag_ADDR(I2C2);
        }
    }

    if (LL_I2C_IsActiveFlag_RXNE(I2C2)) {
        i2c_ex_timeout_start = HAL_GetTick();
        Slave_Reception_Callback();
    }

    if (LL_I2C_IsActiveFlag_STOP(I2C2)) {
        LL_I2C_ClearFlag_STOP(I2C2);

        if (ubReceiveIndex && !rx_overflow) {
            Slave_Complete_Callback((uint8_t *)aReceiveBuffer, ubReceiveIndex);
        }
        i2c2_finish_transfer();
    }

    if (LL_I2C_IsActiveFlag_NACK(I2C2)) {
        LL_I2C_ClearFlag_NACK(I2C2);
        i2c2_finish_transfer();
    }

    if ((i2c_slave_state == I2C_SLAVE_STATE_WRITE) && LL_I2C_IsActiveFlag_TXIS(I2C2)) {
        i2c_ex_timeout_start = HAL_GetTick();
        Slave_Ready_To_Transmit_Callback();
    }
}

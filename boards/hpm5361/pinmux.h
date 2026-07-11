/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PINMUX_H
#define _PINMUX_H

#include "hpm_soc.h"
#include "hpm_mcan_regs.h"

void init_xtal_pins(void);
void init_jtag_pins(void);
void init_uart_pins(UART_Type *ptr);
void init_pwm_pins(PWM_Type *ptr);
void init_key_pins(void);
void init_gpio_pins(void);
void init_spi_pins(SPI_Type *ptr);
void init_i2c_pins(I2C_Type *ptr);
void init_adc16_pins(void);
void init_can_pins(MCAN_Type *ptr);

#endif /* _PINMUX_H */

/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _HPM_BOARD_H
#define _HPM_BOARD_H
#include "hpm_common.h"
#include "hpm_soc.h"
#include "hpm_soc_feature.h"
#include "hpm_clock_drv.h"
#include "pinmux.h"
#include <stdio.h>
#include <stdarg.h>

#define BOARD_NAME "hpm5361"
#define BOARD_UF2_SIGNATURE (0x0A4D5048UL)

/* 调试输出宏定义 */
#define DEBUG_ENABLED 1
#if DEBUG_ENABLED
#define DEBUG_PRINT(fmt, ...) debug_uart_send_string(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

/* DMA控制器定义 */
#define BOARD_APP_HDMA                              HPM_HDMA
#define BOARD_APP_DMAMUX                            HPM_DMAMUX

/* ADC通道定义 */
#define MOTOR_CURRENT_ADC_CH    3
#define MOTOR_VOLTAGE_ADC_CH    2
#define BUS_VOLTAGE_ADC_CH      4

/* 定时器配置 */
#define BOARD_CALLBACK_TIMER     HPM_GPTMR3
#define BOARD_CALLBACK_TIMER_CH  1
#define BOARD_CALLBACK_TIMER_IRQ IRQn_GPTMR3
#define BOARD_CALLBACK_TIMER_CLK_NAME clock_gptmr3

/* I2C参数存储配置 */
#define PARAM_I2C_CTRL          HPM_I2C0
#define PARAM_I2C_CLK_NAME      clock_i2c0
#define PARAM_I2C_FREQ          100000  /* 100kHz */

/* PWM配置 */
#define BOARD_APP_PWM                HPM_PWM0
#define BOARD_APP_PWM_CLOCK_NAME     clock_mot0
#define BOARD_APP_PWM_OUT1          6  // PWM输出通道0
#define BOARD_PWM_PERIOD        1000    /* PWM周期 */
#define MOTOR_PWM_BASE          HPM_PWM0
#define MOTOR_PWM_CH            5
#define MOTOR_PWM_CLK_NAME      clock_ptmr
#define MOTOR_PWM_FREQ          10000   /* 10kHz */
#define MOTOR_PWM_DEADTIME      200     /* 死区时间 200ns */
#define BOARD_APP_TRGM            HPM_TRGM0
#define BOARD_APP_PWM_OUT        6
#define BOARD_APP_PWM_IRQn        IRQn_PWM0

/* 正在运行的核心 */
#define BOARD_RUNNING_CORE      0

/* ADC参考电压定义 */
#define ADC_VREF_MV            3300  /* 3.3V参考电压 */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef void (*board_timer_cb)(void);

void board_init(void);
void board_init_led_pins(void);
void board_led_write(uint8_t pin, uint8_t state);
void board_led_toggle(void);
void board_init_uart(UART_Type *ptr);
uint32_t board_init_uart_clock(UART_Type *ptr);
void board_init_i2c(I2C_Type *ptr);
uint32_t board_init_adc16_clock(ADC16_Type *ptr, bool clk_src_ahb);
void board_init_adc16_pins(uint8_t ch);
void board_init_pwm_pins(PWM_Type *ptr);
void board_init_can(MCAN_Type *ptr);
uint32_t board_init_can_clock(MCAN_Type *ptr);
void board_delay_ms(uint32_t ms);
void board_delay_us(uint32_t us);
void board_timer_create(uint32_t ms, board_timer_cb cb);
uint8_t board_get_led_gpio_off_level(void);
void board_init_key_pins(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _HPM_BOARD_H */

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


/* GPIO控制器定义 */
#define BOARD_APP_GPIO_CTRL     HPM_GPIO0
#define BOARD_APP_GPIO_INDEX    0

/* ADC控制器定义 */
#define BOARD_APP_ADC16         HPM_ADC0
#define BOARD_APP_ADC16_INDEX   0
#define BOARD_APP_ADC16_CLK_NAME clock_adc0

/* 按键定义 */
#define BOARD_KEY1_GPIO         BOARD_APP_GPIO_CTRL
#define BOARD_KEY1_PORT         GPIO_DO_GPIOA
#define BOARD_KEY1_PIN          9

#define BOARD_KEY2_GPIO         BOARD_APP_GPIO_CTRL
#define BOARD_KEY2_PORT         GPIO_DO_GPIOA
#define BOARD_KEY2_PIN          14

/* MODBUS串口定义 */
#define BOARD_MODBUS_UART                           HPM_UART3
#define BOARD_MODBUS_UART_CLK_NAME                  clock_uart3
#define BOARD_MODBUS_UART_BAUDRATE                  (9600UL)
#define BOARD_MODBUS_UART_IRQ                       IRQn_UART3

/* DMA控制器定义 */
#define BOARD_APP_HDMA                              HPM_HDMA
#define BOARD_APP_DMAMUX                            HPM_DMAMUX


/* 调试串口定义 */
#define BOARD_CONSOLE_UART_BASE HPM_UART1
#define BOARD_CONSOLE_UART_CLK_NAME clock_uart1
#define BOARD_CONSOLE_UART_BAUDRATE (115200UL)

/* LoRa串口定义 */
#define BOARD_LORA_UART                             HPM_UART2
#define BOARD_LORA_UART_CLK_NAME                    clock_uart2
#define BOARD_LORA_UART_BAUDRATE                    (9600UL)
#define BOARD_LORA_UART_DMA_IRQ                     IRQn_HDMA
#define BOARD_LORA_UART_DMA_CONTROLLER              HPM_HDMA
#define BOARD_LORA_UART_DMAMUX_CONTROLLER           HPM_DMAMUX
#define BOARD_LORA_UART_TX_DMA_CHN                 (25U)
#define BOARD_LORA_UART_RX_DMA_CHN                 (24U)
#define BOARD_LORA_UART_RX_DMAMUX_CHN              DMA_SOC_CHN_TO_DMAMUX_CHN(BOARD_LORA_UART_DMAMUX_CONTROLLER, BOARD_LORA_UART_RX_DMA_CHN)
#define BOARD_LORA_UART_TX_DMAMUX_CHN              DMA_SOC_CHN_TO_DMAMUX_CHN(BOARD_LORA_UART_DMAMUX_CONTROLLER, BOARD_LORA_UART_TX_DMA_CHN)
#define BOARD_LORA_UART_TX_DMA_REQ                 HPM_DMA_SRC_UART2_TX
#define BOARD_LORA_UART_RX_DMA_REQ                 HPM_DMA_SRC_UART2_RX
#define BOARD_LORA_UART_IRQ                        IRQn_UART2
/* LED定义 */
#define BOARD_LED_GPIO_CTRL     BOARD_APP_GPIO_CTRL
#define BOARD_LED_PORT          GPIO_DO_GPIOA
#define BOARD_LED_PIN           15
#define BOARD_LED_ON_LEVEL      0  /* 低电平点亮 */
#define BOARD_LED_OFF_LEVEL     1

/* 步进电机引脚定义 */
#define BOARD_STEP_GPIO         BOARD_APP_GPIO_CTRL
#define BOARD_STEP_PORT         GPIO_DO_GPIOA
#define BOARD_STEP_PIN          10

#define BOARD_DIR_GPIO          BOARD_APP_GPIO_CTRL
#define BOARD_DIR_PORT          GPIO_DO_GPIOA
#define BOARD_DIR_PIN           12

#define BOARD_EN_GPIO           BOARD_APP_GPIO_CTRL
#define BOARD_EN_PORT           GPIO_DO_GPIOA
#define BOARD_EN_PIN            13

#define BOARD_BACK_GPIO          BOARD_APP_GPIO_CTRL
#define BOARD_BACK_PORT          GPIO_DO_GPIOA
#define BOARD_BACK_PIN           14

#define BOARD_FORWARD_GPIO          BOARD_APP_GPIO_CTRL
#define BOARD_FORWARD_PORT          GPIO_DO_GPIOA
#define BOARD_FORWARD_PIN           9

/* LoRa控制定义 */
#define LORA_GPIO_CTRL          BOARD_APP_GPIO_CTRL
#define LORA_M0_PORT            GPIO_DO_GPIOA
#define LORA_M0_PIN             18
#define LORA_M1_PORT            GPIO_DO_GPIOA
#define LORA_M1_PIN             19
#define LORA_AUX_PORT           GPIO_DO_GPIOA
#define LORA_AUX_PIN            20

/* BLDC驱动器控制引脚定义 */
#define BLDC_GPIO_CTRL          BOARD_APP_GPIO_CTRL
#define BLDC_OUT1_PORT            GPIO_DO_GPIOA
#define BLDC_OUT1_PIN             10
#define BLDC_OUT2_PORT            GPIO_DO_GPIOA
#define BLDC_OUT2_PIN             11

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
/* LED GPIO定义 */
#define BOARD_LED_GPIO_INDEX    0
#define BOARD_LED_GPIO_PIN      BOARD_LED_PIN

/* RS485串口定义 */
#define BOARD_RS485_UART                            HPM_UART0
#define RS485_UART_CLK_NAME                         clock_uart0
#define RS485_UART_BAUDRATE                         (9600UL)
#define RS485_UART_IRQ                             IRQn_UART0

/* RS485方向控制引脚定义 */
#define RS485_DE_GPIO_CTRL                          BOARD_APP_GPIO_CTRL
#define RS485_DE_PORT                               GPIO_DO_GPIOY
#define RS485_DE_PIN                                7
/* RS485 DMA配置 */
#define RS485_UART_TX_DMA_CHN              (21U)
#define RS485_UART_RX_DMA_CHN              (20U)
#define RS485_UART_TX_DMAMUX_CHN           DMA_SOC_CHN_TO_DMAMUX_CHN(BOARD_APP_HDMA, RS485_UART_TX_DMA_CHN)
#define RS485_UART_RX_DMAMUX_CHN           DMA_SOC_CHN_TO_DMAMUX_CHN(BOARD_APP_HDMA, RS485_UART_RX_DMA_CHN)
#define RS485_UART_TX_DMA_REQ              HPM_DMA_SRC_UART0_TX
#define RS485_UART_RX_DMA_REQ              HPM_DMA_SRC_UART0_RX
// #define RS485_UART_IRQ                     IRQn_UART0

/* MAX3485ED控制引脚 */
#define RS485_CTRL_GPIO                    HPM_GPIO0
#define RS485_CTRL_PORT                    GPIO_DO_GPIOY
#define RS485_CTRL_PIN                     7
#define RS485_MODE_RX                      0  // 低电平接收
#define RS485_MODE_TX                      1  // 高电平发送

// 接收缓冲区大小
// #define RS485_RX_BUFFER_SIZE              (128U)
/* RS485缓冲区大小 */
#define RS485_DMA_BUFFER_SIZE              256
#define RS485_IDLE_THRESHOLD               40U  // 空闲检测阈值

/* 正在运行的核心 */
#define BOARD_RUNNING_CORE      0

/* ADC参考电压定义 */
#define ADC_VREF_MV            3300  /* 3.3V参考电压 */

/* CAN配置定义 */
#define BOARD_APP_CAN_BASE                      HPM_MCAN0
#define BOARD_APP_CAN_IRQn                     IRQn_MCAN0
#define BOARD_APP_CAN_DEFAULT_BAUDRATE          (500000UL)  /* 500kbps */

/**
 * @brief 获取系统运行时间(毫秒)
 * 
 * @return uint32_t 系统运行时间(毫秒)
 */
uint32_t board_get_time_ms(void);

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef void (*board_timer_cb)(void);

void board_init(void);
void board_init_gpio_pins(void);
void board_init_led_pins(void);
void board_led_write(uint8_t pin, uint8_t state);
void board_led_toggle(void);
void board_init_uart(UART_Type *ptr);
uint32_t board_init_uart_clock(UART_Type *ptr);
void board_init_i2c(I2C_Type *ptr);
uint32_t board_init_adc16_clock(ADC16_Type *ptr, bool clk_src_ahb);
void board_init_adc16_pins(uint8_t ch);
uint32_t board_init_pwm_clock(PWM_Type *ptr);
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

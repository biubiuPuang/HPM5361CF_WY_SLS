#ifndef __DRV_RELAY_H
#define __DRV_RELAY_H

#include "pinmux.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"

/* 定义LED引脚 */
#define BOARD_RELAY_GPIO_CTRL         HPM_GPIO0
#define BOARD_BUTTON_GPIO_PORT        GPIO_DI_GPIOB  // GPIOB
#define RS485CONTROL_GPIO_PIN         (2U) // 测试板子为PB2，实际板子为PB10
#define CANCONTROL_GPIO_PIN           (3U) // 测试板子为PB3，实际板子为PB11

// 继电器动作状态
typedef enum {
    RELAY_ON = 0,
    RELAY_OFF = 1
} RelayAction_t;

// 继电器通道
typedef enum {
    RELAY_RS485 = 0,
    RELAY_CAN   = 1,
    RELAY_OUT1  = 2,
    RELAY_OUT2  = 3,
    RELAY_NUM
} RelayChannel_t;

typedef struct {
                    //  : 1 表示这个个数据只有1个bit表示
    uint8_t rs485relay : 1;  // 只占用 1 bit，0表示断开，1表示吸合
    uint8_t canrelay : 1;  // 只占用 1 bit
    uint8_t out1relay : 1;
    uint8_t out2relay : 1;
    uint8_t reserved : 4; // 补齐剩下的4位，方便编译器处理（可选）
} RelayState_t;

extern RelayState_t relays;

void init_relay(void);
void control_relay(RelayChannel_t out_ch, RelayAction_t state);

#endif
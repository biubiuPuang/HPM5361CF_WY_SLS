#ifndef _DRV_LED_H
#define _DRV_LED_H

#include "pinmux.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"
/* 定义LED引脚 */
#define BOARD_LED_GPIO_CTRL         HPM_GPIO0
#define BOARD_LED_GPIO_PORT         GPIO_DI_GPIOA
#define BOARD_LED_GPIO_PIN1          (11U)
#define BOARD_LED_GPIO_PIN2          (26U)

#define BOARD_LED_OFF          1  /* 高电平熄灭 */
#define BOARD_LED_ON           0  /* 低电平点亮 */

typedef enum {
    LED_OFF = 0,    // LED灯开启 (0)  LED灯关闭状态 0
    LED_ON        // LED灯关闭 (1)  LED灯开启状态 1
}led_state_t;

/**
 * @brief 初始化LED引脚
 */
void init_led(void);

/**
 * @brief 设置LED引脚电平
 *
 * @param state BOARD_LED_OFF: 关闭LED, BOARD_LED_ON: 打开LED
 */
void set_led_state(uint8_t state);

/**
 * @brief 切换LED引脚电平
 */
void led_toggle(void);

uint8_t get_led_state();

#endif /* _DRV_SYS_LED_H */
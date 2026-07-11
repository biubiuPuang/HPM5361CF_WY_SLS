#ifndef __DRV_BUTTON_H__
#define __DRV_BUTTON_H__

#include "pinmux.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"

/* 定义LED引脚 */
#define BOARD_BUTTON_GPIO_CTRL         HPM_GPIO0
#define BOARD_BUTTON_GPIO_PORT         GPIO_DI_GPIOY  // 测试板子为PY，目标板子为PA
#define BOARD_BUTTON_GPIO_PIN         (6U) // 测试板子为6，目标板子为12

void init_button(void);
uint8_t get_button_state();

#endif /* __DRV_BUTTON_H__ */
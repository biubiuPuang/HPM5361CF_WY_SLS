#include "drv_led.h"

led_state_t led_state;

/**
 * @brief 初始化LED引脚
 * @param void
 * @return void
 */
void init_led(void)
{
    HPM_IOC->PAD[IOC_PAD_PA11].FUNC_CTL = IOC_PA11_FUNC_CTL_GPIO_A_11;

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, BOARD_LED_GPIO_PIN1, gpiom_soc_gpio0);
    gpio_set_pin_output(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN1);
    gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN1, BOARD_LED_OFF);

    HPM_PIOC->PAD[IOC_PAD_PA26].FUNC_CTL = IOC_PA26_FUNC_CTL_GPIO_A_26;
    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOA, BOARD_LED_GPIO_PIN2, gpiom_soc_gpio0);
    gpio_set_pin_output(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN2);
    gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN2, BOARD_LED_OFF);

    led_state = LED_OFF;
}

/**
 * @brief 设置LED引脚电平
 * @param state BOARD_LED_OFF: 关闭LED, BOARD_LED_ON: 打开LED
 * @return void
 */
void set_led_state(uint8_t state)
{
    gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN1, state);
    // gpio_write_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN2, state);
    led_state = !state;
}

/**
 * @brief 切换LED引脚电平
 * @param void
 * @return void
 */
void led_toggle(void)
{
    gpio_toggle_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN1);
    // gpio_toggle_pin(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN2);
    led_state = !led_state;
}

void sys_error(void)
{
    led_toggle();
    board_delay_ms(500);
}

/**
 * @brief 获取LED状态
 * @param void
 * @return uint8_t LED状态
*/
uint8_t get_led_state()
{
    return led_state;
}

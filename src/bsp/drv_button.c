#include "drv_button.h"

void init_button(void)
{
    HPM_IOC->PAD[IOC_PAD_PA12].FUNC_CTL = IOC_PA12_FUNC_CTL_GPIO_A_12; 

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOY, BOARD_BUTTON_GPIO_PIN, gpiom_soc_gpio0);
    gpio_set_pin_input(BOARD_BUTTON_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, BOARD_BUTTON_GPIO_PIN);
    gpio_disable_pin_interrupt(BOARD_BUTTON_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, BOARD_BUTTON_GPIO_PIN);
}

uint8_t get_button_state(void)
{
    return gpio_read_pin(BOARD_BUTTON_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, BOARD_BUTTON_GPIO_PIN);
}
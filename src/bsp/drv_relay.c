#include "drv_relay.h"

RelayState_t relays;

void init_relay(void)
{
    HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PB10_FUNC_CTL_GPIO_B_10;

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOB, RS485CONTROL_GPIO_PIN, gpiom_soc_gpio0);
    gpio_set_pin_output(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, RS485CONTROL_GPIO_PIN);
    gpio_write_pin(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, RS485CONTROL_GPIO_PIN, RELAY_OFF);

    HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PB11_FUNC_CTL_GPIO_B_11;

    gpiom_set_pin_controller(HPM_GPIOM, GPIOM_ASSIGN_GPIOB, CANCONTROL_GPIO_PIN, gpiom_soc_gpio0);
    gpio_set_pin_output(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, CANCONTROL_GPIO_PIN);
    gpio_write_pin(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, CANCONTROL_GPIO_PIN, RELAY_OFF);   

    relays.rs485relay = 0;
    relays.canrelay = 0;
}

void control_relay(RelayChannel_t out_ch, RelayAction_t state)
{
    switch (out_ch) {
    case RELAY_RS485:
        gpio_write_pin(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, RS485CONTROL_GPIO_PIN, state);
        relays.rs485relay = state;
        break;
    case RELAY_CAN :
        gpio_write_pin(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, CANCONTROL_GPIO_PIN, state);
        relays.canrelay = state;
        break;
    }
}

void relay_set_all(bool state)
{
    if (state) {
        gpio_write_pin(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, RS485CONTROL_GPIO_PIN, !state);
        gpio_write_pin(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, CANCONTROL_GPIO_PIN, !state);
        relays.out1relay = state;
        relays.out2relay = state;
    } else {
        gpio_write_pin(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, RS485CONTROL_GPIO_PIN, !state);
        gpio_write_pin(BOARD_RELAY_GPIO_CTRL, BOARD_BUTTON_GPIO_PORT, CANCONTROL_GPIO_PIN, !state);
        relays.out1relay = state;
        relays.out2relay = state;
    }
}

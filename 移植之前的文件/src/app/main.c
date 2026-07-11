


// /**
//  * @file main.c
//  * @brief 主程序入口
//  * @note 1.0版本
//  * 基于HPM5361ICF1的双踏板监控主板
//  */

// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// /* 系统头文件 */
// #include "board.h"
// // #include "board_patch.h"
// #include "hpm_uart_drv.h"
// #include "hpm_gpio_drv.h"
// #include "hpm_clock_drv.h"
// #include "hpm_sysctl_drv.h"
// #include "pinmux.h"

// /* 外设头文件 */
// #include "drv_led.h"
// #include "drv_button.h"
// #include "drv_relay.h"
// #include "drv_rs485.h"

// static hpm_stat_t system_init(void)
// {
//     // hpm_stat_t 官方库函数自带的初始化成功与否的标志位
//     // 这里的返回值默认是初始化成功的
//     hpm_stat_t status = status_success;
//     board_init();  // 板级初始化
//     init_led();    // 初始化LED
//     init_button(); // 初始化按键
//     init_relay();  // 初始化继电器
//     // ***Bug---如果第一路485初始化失败，但第二路成功，最后还是会返回成功。
//     // status = rs485_init(RS485_CH1); // 初始化第一路RS485通讯
//     // status = rs485_init(RS485_CH2); // 初始化第二路RS485通讯

//     // 修正后的代码
//     status = rs485_init(RS485_CH1); // 初始化第一路的RS485通讯
//     // 判断第一路的RS485通讯初始化是否成功
//     if (status != status_success)
//     {
//         return status;
//     }

//     status = rs485_init(RS485_CH2); // 初始化第二路的RS485通讯
//     // 判断第二路的RS485通讯初始化是否成功
//     if (status != status_success)
//     {
//         return status;
//     }

//     // 如果程序能执行到这说明两路的RS485通讯初始化都成功了
//     return status;
// }

/**
 * @brief 主程序入口
 */

// int main(void)
// {
//     //       status  != status_success
//    if (system_init() != status_success) {
//        while (1) {
//             // 翻转LED-表示初始化失败
//            led_toggle();
//            board_delay_ms(500);
//        }
//    }
//     /* 主循环 */
//     while (1) {
//         // 获取当前按键的高低电平
//         uint8_t stateled = get_button_state();
//         if (stateled == 1) {
//             // 每500ms翻转一次LED灯的状态
//             led_toggle();
//             board_delay_ms(500);

//             // 参数1:选择RS485 参数2:关闭继电器
//             control_relay(RELAY_RS485, RELAY_ON);
//             board_delay_ms(500);
//             control_relay(RELAY_CAN, RELAY_ON);

//         } else {
//             // 关闭LED 随后关闭 RS485 和CAN
//             set_led_state(BOARD_LED_ON);
//             control_relay(RELAY_RS485, RELAY_OFF);
//             control_relay(RELAY_CAN, RELAY_OFF);
//         }

//         app_rs485_poll();
//     }

//     return 0;
// }


/**
 * @file main.c
 * @brief 主程序入口
 * @note 1.0版本
 * 基于HPM5361ICF1的双踏板监控主板
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 系统头文件 */
#include "board.h"
// #include "board_patch.h"
#include "hpm_uart_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_sysctl_drv.h"
#include "pinmux.h"

/* 外设头文件 */
#include "drv_led.h"
#include "drv_button.h"
#include "drv_relay.h"
#include "drv_rs485.h"

static hpm_stat_t system_init(void)
{
    // hpm_stat_t 官方库函数自带的初始化成功与否的标志位
    // 这里的返回值默认是初始化成功的
    hpm_stat_t status = status_success;
    board_init();  // 板级初始化
    init_led();    // 初始化LED
    init_button(); // 初始化按键
    init_relay();  // 初始化继电器
    // ***Bug---如果第一路485初始化失败，但第二路成功，最后还是会返回成功。
    // status = rs485_init(RS485_CH1); // 初始化第一路RS485通讯
    // status = rs485_init(RS485_CH2); // 初始化第二路RS485通讯

    // 修正后的代码
    status = rs485_init(RS485_CH1); // 初始化第一路的RS485通讯
    // 判断第一路的RS485通讯初始化是否成功
    if (status != status_success)
    {
        return status;
    }

    status = rs485_init(RS485_CH2); // 初始化第二路的RS485通讯
    // 判断第二路的RS485通讯初始化是否成功
    if (status != status_success)
    {
        return status;
    }

    // 如果程序能执行到这说明两路的RS485通讯初始化都成功了
    return status;
}

int main(void)
{
    //       status  != status_success
    if (system_init() != status_success)
    {
        while (1)
        {
            // 翻转LED-表示初始化失败
            led_toggle();
            board_delay_ms(500);
        }
    }

    whiel(1)
    {

    }
    
    return 0;
}

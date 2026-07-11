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
#include "drv_timer_extra.h"

/* 双踏板控制相关的头文件 */

/* 系统定时器计数器 */
static uint32_t system_tick_counter = 0;
volatile uint32_t monitor_timeout = 0; // 监控超时计数器
volatile bool busTimeoutFlag = false;  // 总线超时标志
volatile bool eStopActive = false;     // 紧急标志  == 1急停被按下  == 0 没有进入急停模式
static bool relayEnergized = false;
#define MOTOR_TIMEOUT_THRESHOLD 5000

// 双踏板相关配置\标志位
/*
 * 安全回退位置：
 * 0    ：更接近逻辑缩回端
 * -500 ：README 里提到的待机偏置候选值
 *
 * 最终值必须现场验证，确认气缸确实收回且不会撞限位。
 */
#define PEDAL_SAFE_RETRACT_CNT          (0)
// 回退失败后的重试间隔，单位ms
#define PEDAL_RETRACT_RETRY_INTERVAL_MS (500)
// 双踏板控制器对象 
static PedalController g_pedal_controller;
static PedalControllerConfig g_pedal_config;
// 双踏板控制器是否已经打开标志位
static bool g_pedal_controller_opened = false;
// 电机/气缸是否已经回退到安全位置标志位
static bool g_motor_at_origin = false;
// 最近一次回退结果
static PedalResult g_pedal_last_result = PEDAL_OK;
// 下一次允许重试回退的系统
static uint32_t g_next_retract_retry_tick = 0;

// 结构体 用于
typedef enum
{
    STATE_NORMAL = 0, // 监听模式
    STATE_SAFETY      // 异常状态
} SysState_t;
static SysState_t sysState = STATE_NORMAL;

/* 定时器间隔配置 */
#define SYSTEM_TIMER_INTERVAL 1 // 1ms定时器中断周期

/* 定时器回调函数 */
static void timer_callback(void)
{
    system_tick_counter++;
    monitor_timeout++;

    if (monitor_timeout >= MOTOR_TIMEOUT_THRESHOLD)
    {
        monitor_timeout = MOTOR_TIMEOUT_THRESHOLD;
        busTimeoutFlag = true;
    }
}

static hpm_stat_t system_init(void)
{
    // hpm_stat_t 官方库函数自带的初始化成功与否的标志位
    // 这里的返回值默认是初始化成功的
    hpm_stat_t status = status_success;
    board_init();
    init_led();
    init_button();
    init_relay();

    // 初始化系统定时器并注册回调
    status = system_timer_init(SYSTEM_TIMER_INTERVAL);
    if (status != status_success)
    {
        printf("系统定时器初始化失败\r\n");
        return status;
    }
    system_timer_register_callback(timer_callback);

    // 初始化RS485通道1
    // 随后里面判断初始化是否成功,不成功直接返回
    status = rs485_init(RS485_CH1);
    if (status != status_success)
    {
        printf("系统定时器RS485_CH1初始化失败\r\n");
        return status;
    }
    // 初始化RS485通道2
    // 随后里面判断初始化是否成功,不成功直接返回
    status = rs485_init(RS485_CH2);
    if (status != status_success)
    {
        printf("系统定时器RS485_CH2初始化失败\r\n");
        return status;
    }

    // 代码运行到这则说明所有初始化功能全部运行成功
    return status;
}

/**
 * @brief 主程序入口
 */
// app_rs485_poll()：用户自定义函数
// rs485_check_and_handle_rx()：用户自定义 RS485 接收检查函数
// app_parse_frame()：用户自定义 RS485 数据处理函数
int main(void)
{
    //       status  != status_success
    if (system_init() != status_success)
    {
        while (1)
        {
            // 翻转LED - 用于表示初始化失败
            led_toggle();
            board_delay_ms(500);
        }
    }
    set_led_state(LED_OFF);
    /* 主循环 */
    while (1)
    {
        eStopActive = get_button_state(); // 获取ESTOP按钮状态

        // 检查两路485总线有没有接受到数据,如果有的话就调用自定义的函数处理数据
        app_rs485_poll();

        // sysState == 状态机的标志位,初始值为监听模式
        switch (sysState)
        {
        // 监听模式进入的循环
        case STATE_NORMAL:
            // 判断,如果急停按钮被触发或者5秒没收到数据就将监听模式改为异常状态,不满意则直接退出
            if (eStopActive || busTimeoutFlag)
                sysState = STATE_SAFETY;
            break;
        
        // 如果为异常状态
        case STATE_SAFETY:
            // 判断继电器是否被吸合,如果没有被吸合则进入循环
            if (!relayEnergized)
            {
                // 吸合继电器
                relay_set_all(true);
                // 吸合继电器的标志位
                relayEnergized = true;
            }

            // 处理异常状态的代码逻辑


            // bus2_tx_retract();                          // 第二路持续命令回退
            bool motorAtOrigin = 1;
            // motor_is_at_origin(); // CH1 检测到原点 = 检测到数据

            // 判断外部主机是否有数据重新发送过来
            bool busRecovered = !busTimeoutFlag; // CH1 有数据 = !超时

            // 急停没有被按下执行if里面的代码
            if (!eStopActive)
            {
                // motorAtOrigin == 电机已经回到原点 用标志位1表示 and busRecovered == 外部主机传来数据 用标志位1表示
                if (motorAtOrigin && busRecovered)
                {
                    // 断开继电器将控制权还给外部主机
                    relay_set_all(false);
                    // 把表示继电器是否吸合的标志位改为 0 == 此时继电器没有吸合
                    relayEnergized = false;
                    // 把表示系统状态的枚举类型改为监听模式
                    sysState = STATE_NORMAL;
                }
            }
            break;
        }
    }

    return 0;
}

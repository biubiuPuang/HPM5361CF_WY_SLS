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
#include "pedal_controller.h"

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
#define PEDAL_SAFE_RETRACT_CNT (0)
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

// 双踏板自定义函数
// safety_open_pedal_controller_once() 配置并打开双踏板控制器函数，只初始化一次。
// safety_retract_pedal_once_or_retry() 控制双踏板回退到原点；失败后隔一段时间继续重试，成功才停止。
// safety_close_pedal_controller() 停止电机、关闭控制器，并清空相关状态。
/**
 * @brief 打开双踏板控制器
 *
 * pedal_controller_default_config()：Python-C_Second-Time 用户自定义配置函数
 * pedal_controller_init()：Python-C_Second-Time 用户自定义初始化函数
 * pedal_controller_open()：Python-C_Second-Time 用户自定义打开函数
 */
static PedalResult safety_open_pedal_controller_once(void)
{
    PedalResult result;

    if (g_pedal_controller_opened)
    {
        return PEDAL_OK;
    }

    /*
     * "RS485_CH2" 这个字符串只是给你的 HPM 适配层识别用。
     * 你后面适配 rs485_transport 时，可以让它固定使用 RS485_CH2。
     */
    g_pedal_config = pedal_controller_default_config("RS485_CH2"); // ❌

    /*
     * 必须现场标定：
     * 0 或 -500 只是候选值。
     */
    g_pedal_config.safe_retract_cnt = PEDAL_SAFE_RETRACT_CNT;

    /*
     * 注意：
     * Python-C_Second-Time 默认波特率是 38400。
     * 你当前 drv_rs485.h 里 RS485_CH2 是 9600。 ✅已将当前项目工程的RS485_CH2修改为38400
     * 最终必须和 IDS830ABS 驱动器参数一致。
     */
    g_pedal_config.baudrate = 38400;

    /*
     * 调试初期建议低速一点，现场确认没问题后再提高。
     * README 默认是 3000。✅有修改
     */
    g_pedal_config.move_max_rpm = 3000;

    // 参数1: 双踏板控制器对象
    // 参数2: 将配置的如波特率,踏板ID,转速等传过去
    result = pedal_controller_init(&g_pedal_controller, &g_pedal_config);
    if (result != PEDAL_OK)
    {
        printf("初始化1错误");
        return result;
    }

    // 打开双踏板控制器
    // 根据g_pedal_config的配置,初始化双踏板
    result = pedal_controller_open(&g_pedal_controller);
    if (result != PEDAL_OK)
    {
        printf("初始化2,踏板控制权打开失败");
        return result;
    }

    // 运行到这说明这个函数里面的初始化全都成功了
    g_pedal_controller_opened = true;
    return PEDAL_OK;
}

/**
 * @brief 执行一次双踏板安全回退
 *
 * pedal_controller_retract_to_safe()：Python-C_Second-Time 用户自定义安全回退函数
 *
 * wait = 1：
 * 函数会等待刹车和油门两个轴都到位后才返回。
 * 这符合你“回退到原点必须完成”的要求。
 */
// 具体执行双踏板电机回退的函数,间隔时间500ms
static void safety_retract_pedal_once_or_retry(void)
{
    // 存标志位的枚举类型
    PedalResult result;

    // 判断双踏板已经回退就直接退出函数
    if (g_motor_at_origin)
    {
        return;
    }

    // 查看下次回退的软件定时器是否到了,到了就发送回退命令
    if (system_tick_counter < g_next_retract_retry_tick)
    {
        return;
    }

    // 查看双踏板的控制器是否被打开,如果已被打开则不再次执行打开控制权的操作
    result = safety_open_pedal_controller_once();
    // 判断双踏板控制器已被打开,执行电机回退到原点的效果
    if (result == PEDAL_OK)
    {
        result = pedal_controller_retract_to_safe(&g_pedal_controller, 1);
    }

    // result执行回退是否成功的标志位
    // g_pedal_last_result把刚刚的将标志位保存起来
    g_pedal_last_result = result;

    // 判断电机回退执行是否成功
    if (result == PEDAL_OK)
    {
        g_motor_at_origin = true;
    }
    // 电机回退失败
    else
    {
        /*
         * 回退失败不能释放继电器。
         * 因为你的需求是：回退到原点是强制要求，必须完成。
         */
        // 判断踏板控制器现在是否还是打开状态
        if (g_pedal_controller_opened)
        {
            // 先停止两个双踏板稳定下来,避免双踏板前后乱跑
            (void)pedal_controller_stop_all(&g_pedal_controller);
        }

        // 设置下一次重试时间 重试间隔500ms
        g_next_retract_retry_tick = system_tick_counter + PEDAL_RETRACT_RETRY_INTERVAL_MS;
    }
}

/**
 * @brief 释放/关闭双踏板控制器,
 *
 *
 *
 * 并把双踏板相关状态清零
 *
 * pedal_controller_stop_all()：Python-C_Second-Time 用户自定义双轴缓停函数
 * pedal_controller_close()：Python-C_Second-Time 用户自定义关闭函数
 */
// 用于退出异常状态恢复成监听模式
// 此函数需改进确认
static void safety_close_pedal_controller(void)
{
    // 判断踏板是否已被打开
    if (g_pedal_controller_opened)
    {
        // 调用双踏板停止函数，让两个踏板电机停止工作
        // 前面的 (void) 是为了表示：我知道这个函数有返回值，但这里我故意不处理它。
        // 也就是说，即使停止失败，代码仍然会继续往下执行关闭操作。
        (void)pedal_controller_stop_all(&g_pedal_controller);
        // 关闭双踏板控制权 如:关闭 RS485 通信 清理控制器状态...
        pedal_controller_close(&g_pedal_controller);
    }

    // 以下是一些标志位,将这些标志位重新恢复原本成的监听模式
    g_pedal_controller_opened = false;
    g_motor_at_origin = false;
    g_pedal_last_result = PEDAL_OK;
    g_next_retract_retry_tick = 0;
}

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
    status = old_rs485_init(RS485_CH1);
    if (status != status_success)
    {
        //  printf("系统定时器RS485_CH1初始化失败\r\n");
        return status;
    }
    // 初始化RS485通道2
    // 随后里面判断初始化是否成功,不成功直接返回
    status = old_rs485_init(RS485_CH2);
    if (status != status_success)
    {
        // printf("系统定时器RS485_CH2初始化失败\r\n");
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
          // 1. 急停按键消抖 + LED非阻塞显示
        static uint32_t estop_release_tick = 0;
        static uint32_t led_blink_tick = 0;

        /* 急停按键消抖：
         * 按下立即生效，松开稳定50ms后才解除
         */
        if (get_button_state() == 1)
        {
            eStopActive = true;
            estop_release_tick = system_tick_counter;
        }
        else if ((system_tick_counter - estop_release_tick) >= 50)
        {
            eStopActive = false;
        }

        /* 急停指示灯：
         * 急停时500ms闪烁一次
         * 正常时常亮
         * 全程不使用 board_delay_ms，不阻塞CPU
         */
        if (eStopActive)
        {
            if ((system_tick_counter - led_blink_tick) >= 500)
            {
                led_blink_tick = system_tick_counter;
                led_toggle();
            }
        }
        else
        {
            set_led_state(BOARD_LED_ON);
        }

        // 检查两路485总线有没有接受到数据,如果有的话就调用自定义的函数处理数据
        // 原本是检查两路,但是这里目前只监听一路RS485_CH1的数据有没有传过来
        app_rs485_poll();

        // sysState == 状态机的标志位,初始值为监听模式
        switch (sysState)
        {
        // 监听模式
        case STATE_NORMAL:
            // // 外接指示灯显示关闭 == 正常监听模式
            // set_led_state(BOARD_LED_ON);

            // 判断,如果急停按钮被触发或者5秒没收到数据就将监听模式改为异常状态,不满意则直接退出
            if (eStopActive || busTimeoutFlag)
                sysState = STATE_SAFETY;
            break;

        // 处理异常状态的代码逻辑
        // 如果为异常状态
        case STATE_SAFETY:
            // 判断继电器是否被吸合,如果没有的话吸合继电器且切换到CH2控制
            if (!relayEnergized)
            {
                // 吸合继电器
                relay_set_all(true);
                // 吸合继电器的标志位
                relayEnergized = true;

                // 每次进入异常状态这里都执行一次回退的逻辑
                // 清除原定回退到点的标志位
                g_motor_at_origin = false;
                // 清除回退原点的软件定时器
                g_next_retract_retry_tick = 0;
            }

            // 判断外部主机CH1是否有数据重新发送过来
            bool busRecovered = !busTimeoutFlag; // CH1 有数据 = !超时

            // 急停没有被按下且CH1没收到数据执行if里面的代码
            if (!eStopActive && busRecovered)
            {
                // // 外接指示灯闪烁
                // set_led_state(BOARD_LED_ON);
                // board_delay_ms(500);
                // set_led_state(BOARD_LED_OFF);
                // board_delay_ms(500);

                // 退出异常状态前先关闭双踏板控制器
                safety_close_pedal_controller();
                // 断开CH2继电器将控制权还给外部主机
                relay_set_all(false);
                // 把表示继电器是否吸合的标志位改为 0 == 此时继电器没有吸合
                relayEnergized = false;
                // 把表示系统状态的枚举类型改为监听模式
                sysState = STATE_NORMAL;
                break;
            }

            // 只有在CH1没恢复的时候才执行双踏板回退的指令
            // 控制双踏板回退
            // 回退成功以后 g_motor_at_origin 会变成 true
            // 回退失败不会释放继电器，会隔 500ms 重试
            safety_retract_pedal_once_or_retry();
            break;
        }
    }

    return 0;
}

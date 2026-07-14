#include "TB_Two_Driver.h"

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
 * @param  PedalResult 返回值 PEDAL_OK == 成功  其他 == 具体报错原因
 * pedal_controller_default_config()：Python-C_Second-Time 用户自定义配置函数
 * pedal_controller_init()：Python-C_Second-Time 用户自定义初始化函数
 * pedal_controller_open()：Python-C_Second-Time 用户自定义打开函数
 */
PedalResult safety_open_pedal_controller_once(void)
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
 * @param 判断回退是否成功
 * pedal_controller_retract_to_safe()：Python-C_Second-Time 用户自定义安全回退函数
 *
 * wait = 1：
 * 函数会等待刹车和油门两个轴都到位后才返回。
 * 这符合你“回退到原点必须完成”的要求。
 */
// 具体执行双踏板电机回退的函数,间隔时间500msPedalResult
bool safety_retract_pedal_once_or_retry(uint32_t now_tick)
{
    // 存标志位的枚举类型,表示是否回退成功
    PedalResult result;

    // 判断双踏板已经回退就直接退出函数
    if (g_motor_at_origin)
    {
        return true;
    }

    // 查看下次回退的软件定时器是否到了,到了就发送回退命令
    if (now_tick < g_next_retract_retry_tick)
    {
        // 进入判断则表示时间还没到
        return false;
    }

    // 查看双踏板的控制器是否被打开,如果已被打开则不再次执行打开控制权的操作
    result = safety_open_pedal_controller_once();
    // 判断双踏板控制器已被打开,如果已经打开则执行电机回退到原点
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
        return true;
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

        // 设置下一次重试时间 重试间隔500ms 避免一直疯狂重复发送回退命令
        g_next_retract_retry_tick = now_tick + PEDAL_RETRACT_RETRY_INTERVAL_MS;

        // 回退成功标志位
        return false;
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
// bool safety_close_pedal_controller(void)
// {
//     // 判断踏板是否已被打开
//     if (g_pedal_controller_opened)
//     {
//         // 调用双踏板停止函数，让两个踏板电机停止工作
//         // 前面的 (void) 是为了表示：我知道这个函数有返回值，但这里我故意不处理它。
//         // 也就是说，即使停止失败，代码仍然会继续往下执行关闭操作。
//         (void)pedal_controller_stop_all(&g_pedal_controller);
//         // 关闭双踏板控制权 如:关闭 RS485 通信 清理控制器状态...
//         pedal_controller_close(&g_pedal_controller);
//     }

//     // 以下是一些标志位,将这些标志位重新恢复原本成的监听模式
//     g_pedal_controller_opened = false;
//     g_motor_at_origin = false;
//     g_pedal_last_result = PEDAL_OK;
//     g_next_retract_retry_tick = 0;
// }


/**
 * @brief 释放/关闭双踏板控制器,
 *
 * @param bool 判断退出是否成功
 *
 * 并把双踏板相关状态清零
 *
 * pedal_controller_stop_all()：Python-C_Second-Time 用户自定义双轴缓停函数
 * pedal_controller_close()：Python-C_Second-Time 用户自定义关闭函数
 */
// 用于退出异常状态恢复成监听模式
bool safety_close_pedal_controller(void)
{
    // 保存停止双踏板的执行结果，默认认为成功
    PedalResult stop_result = PEDAL_OK;

    // 判断踏板是否已被打开
    if (g_pedal_controller_opened)
    {
        // 调用双踏板停止函数，让两个踏板电机停止工作
        // 利用标志位stop_result 判断回退结果如何
        // 也就是说，即使停止失败，代码仍然会继续往下执行关闭操作。
        stop_result = pedal_controller_stop_all(&g_pedal_controller);
        // 关闭双踏板控制权 如:关闭 RS485 通信 清理控制器状态...
        pedal_controller_close(&g_pedal_controller);
    }

    // 以下是一些标志位,将这些标志位重新恢复原本成的监听模式
    g_pedal_controller_opened = false;
    g_motor_at_origin = false;
    g_next_retract_retry_tick = 0;

    // 利用标志位 判断返回是否成功
    if (stop_result != PEDAL_OK)
    {
        g_pedal_last_result = stop_result;
        return false;
    }
    // 停止成功，或者原本就没打开，认为关闭流程成功
    g_pedal_last_result = PEDAL_OK;
    return true;
}

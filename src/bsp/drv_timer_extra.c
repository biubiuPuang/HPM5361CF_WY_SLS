#include "drv_timer_extra.h"

/* 定时器配置 */
#define SYSTEM_TIMER                BOARD_CALLBACK_TIMER
#define SYSTEM_TIMER_CH             BOARD_CALLBACK_TIMER_CH
#define SYSTEM_TIMER_IRQ            BOARD_CALLBACK_TIMER_IRQ
#define SYSTEM_TIMER_CLK            BOARD_CALLBACK_TIMER_CLK_NAME

/* 系统时间计数器 */
static volatile uint32_t system_ticks = 0;

/* 定时器中断标志 */
static volatile bool timer_triggered = false;

/* 定时器回调函数 */
static timer_callback_t timer_callback = NULL;

/**
 * @brief 定时器中断处理函数
 */
void system_timer_isr(void)
{
    /* 检查定时器中断状态 */
    if (gptmr_check_status(SYSTEM_TIMER, GPTMR_CH_RLD_STAT_MASK(SYSTEM_TIMER_CH))) {
        /* 清除中断标志 */
        gptmr_clear_status(SYSTEM_TIMER, GPTMR_CH_RLD_STAT_MASK(SYSTEM_TIMER_CH));
        
        /* 更新系统计数器 */
        system_ticks++;

        /* 设置触发标志 */
        timer_triggered = true;
        
        /* 执行回调函数 */
        if (timer_callback != NULL) {
            timer_callback();
        }
    }
}
SDK_DECLARE_EXT_ISR_M(SYSTEM_TIMER_IRQ, system_timer_isr)

/**
 * @brief 初始化系统定时器
 * 
 * @param period_ms 定时周期(毫秒)
 * @return hpm_stat_t 状态码
 */
hpm_stat_t system_timer_init(uint32_t period_ms)
{
    /* 重置计数器 */
    system_ticks = 0;
    timer_triggered = false;
    
    /* 定时器配置 */
    uint32_t timer_freq;
    gptmr_channel_config_t config;
    
    /* 获取默认配置 */
    gptmr_channel_get_default_config(SYSTEM_TIMER, &config);
    
    /* 获取时钟频率 */
    timer_freq = clock_get_frequency(SYSTEM_TIMER_CLK);
    
    /* 设置重载值(定时周期) */
    config.reload = timer_freq / 1000 * period_ms;
    
    /* 配置定时器通道 */
    if (gptmr_channel_config(SYSTEM_TIMER, SYSTEM_TIMER_CH, &config, false) != status_success) {
        return status_fail;
    }
    
    /* 启动定时器 */
    gptmr_start_counter(SYSTEM_TIMER, SYSTEM_TIMER_CH);
    
    /* 使能中断 */
    gptmr_enable_irq(SYSTEM_TIMER, GPTMR_CH_RLD_IRQ_MASK(SYSTEM_TIMER_CH));
    intc_m_enable_irq_with_priority(SYSTEM_TIMER_IRQ, 1);
    
    return status_success;
}

/**
 * @brief 注册定时器回调函数
 * 
 * @param callback 回调函数指针
 */
void system_timer_register_callback(timer_callback_t callback)
{
    timer_callback = callback;
}

/**
 * @brief 获取系统时间(毫秒)
 * 
 * @return uint32_t 系统运行时间(毫秒)
 */
uint32_t system_get_time_ms(void)
{
    return system_ticks;
}

/**
 * @brief 获取定时器中断标志
 * 
 * @return bool 是否发生定时器中断
 */
bool system_timer_is_triggered(void)
{
    return timer_triggered;
}

/**
 * @brief 清除定时器中断标志
 */
void system_timer_clear_trigger(void)
{
    timer_triggered = false;
}

#ifndef _DRV_TIMER_EXTRA_H
#define _DRV_TIMER_EXTRA_H

#include "board.h"
#include "hpm_sysctl_drv.h"
#include "hpm_gptmr_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_gptmr_drv.h"

/**
 * @brief 定时器回调函数类型
 */
typedef void (*timer_callback_t)(void);

/**
 * @brief 初始化系统定时器
 * 
 * @param period_ms 定时周期(毫秒)
 * @return hpm_stat_t 状态码
 */
hpm_stat_t system_timer_init(uint32_t period_ms);

/**
 * @brief 注册定时器回调函数
 * 
 * @param callback 回调函数指针
 */
void system_timer_register_callback(timer_callback_t callback);

/**
 * @brief 获取系统时间(毫秒)
 * 
 * @return uint32_t 系统运行时间(毫秒)
 */
uint32_t system_get_time_ms(void);

/**
 * @brief 获取定时器中断标志
 * 
 * @return bool 是否发生定时器中断
 */
bool system_timer_is_triggered(void);

/**
 * @brief 清除定时器中断标志
 */
void system_timer_clear_trigger(void);

#endif /* _DRV_TIMER_EXTRA_H */

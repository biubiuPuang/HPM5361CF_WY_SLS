#ifndef __DRV_WATCHDOG_H__
#define __DRV_WATCHDOG_H__

#include <stdint.h>
#include "hpm_common.h"

/**
 * @brief 初始化看门狗
 *
 * init_watchdog()：用户自定义 BSP 看门狗初始化函数
 */
hpm_stat_t init_watchdog(uint32_t timeout_ms);

/**
 * @brief 喂狗
 *
 * watchdog_feed()：用户自定义 BSP 喂狗函数
 */
void watchdog_feed(void);

#endif

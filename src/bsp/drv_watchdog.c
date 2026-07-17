#include "drv_watchdog.h"
#include "hpm_ewdg_drv.h"
#include "hpm_clock_drv.h"

/*
 * HPM EWDG 外部低速时钟，官方示例里用 32768Hz。
 */
#define WATCHDOG_CLK_FREQ_HZ (32768UL)

hpm_stat_t init_watchdog(uint32_t timeout_ms)
{
    ewdg_config_t config;
    hpm_stat_t status;

    /*
     * clock_add_to_group()：HPM 官方时钟函数
     * 你项目的 board.c 里已经加过 clock_watchdog0，
     * 这里保留是为了保证看门狗时钟一定开启。
     */
    clock_add_to_group(clock_watchdog0, 0);

    /*
     * ewdg_get_default_config()：HPM 官方 EWDG 默认配置函数
     */
    ewdg_get_default_config(HPM_EWDG0, &config);

    /*
     * 开启看门狗
     */
    config.enable_watchdog = true;

    /*
     * 使用微秒单位配置超时时间，不直接手算 tick
     */
    config.ctrl_config.use_lowlevel_timeout = false;

    /*
     * 超时后复位芯片
     */
    config.int_rst_config.enable_timeout_reset = true;

    /*
     * 使用外部低速时钟
     */
    config.ctrl_config.cnt_clk_sel = ewdg_cnt_clk_src_ext_osc_clk;
    config.cnt_src_freq = WATCHDOG_CLK_FREQ_HZ;

    /*
     * timeout_ms 转成 us
     */
    config.ctrl_config.timeout_reset_us = timeout_ms * 1000UL;

    /*
     * ewdg_init()：HPM 官方 EWDG 初始化函数
     */
    status = ewdg_init(HPM_EWDG0, &config);
    if (status == status_success)
    {
        /*
         * 初始化成功后先喂一次狗
         */
        (void)ewdg_refresh(HPM_EWDG0);
    }

    return status;
}

void watchdog_feed(void)
{
    /*
     * ewdg_refresh()：HPM 官方喂狗函数
     */
    (void)ewdg_refresh(HPM_EWDG0);
}

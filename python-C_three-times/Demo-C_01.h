#ifndef DEMO_C_01_H
#define DEMO_C_01_H

/*
 * HPM5361CF + IDS830ABS 双轴 RS485 控制模块。
 *
 * 前置条件：
 * 1. BSP 已初始化目标 UART 的时钟、引脚复用和 38400 8N1。
 * 2. UART 转 RS485 模块默认自动收发；手动 DE/RE 时配置方向回调。
 * 3. 首次实机测试先降低回零电流并确认 home_torque_sign。
 * 4. 所有控制 API 为阻塞调用；每根轴回零最长可阻塞约 60 秒。
 */

#include <stdbool.h>
#include <stdint.h>
#include "hpm_uart_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum demo_c01_result {
    DEMO_C01_OK = 0,
    DEMO_C01_ERR_NULL = -1,
    DEMO_C01_ERR_NOT_INITIALIZED = -2,
    DEMO_C01_ERR_NOT_HOMED = -3,
    DEMO_C01_ERR_PARAM = -4,
    DEMO_C01_ERR_SOFT_LIMIT = -5,
    DEMO_C01_ERR_UART_TX = -6,
    DEMO_C01_ERR_UART_RX = -7,
    DEMO_C01_ERR_UART_TIMEOUT = -8,
    DEMO_C01_ERR_CRC = -9,
    DEMO_C01_ERR_RESPONSE = -10,
    DEMO_C01_ERR_MODBUS_EXCEPTION = -11,
    DEMO_C01_ERR_SERVO_FAULT = -12,
    DEMO_C01_ERR_HOMING_TIMEOUT = -13,
    DEMO_C01_ERR_HOMING_TRAVEL = -14,
    DEMO_C01_ERR_HOMING_CURRENT = -15,
    DEMO_C01_ERR_MOVE_TIMEOUT = -16,
    DEMO_C01_ERR_DUAL_DIFFERENCE = -17,
    DEMO_C01_ERR_SDK_ADAPTATION = -18
} demo_c01_result_t;

typedef enum demo_c01_rs485_direction_mode {
    DEMO_C01_RS485_AUTO_DIRECTION = 0,
    DEMO_C01_RS485_CALLBACK_DIRECTION = 1
} demo_c01_rs485_direction_mode_t;

typedef void (*demo_c01_direction_callback_t)(void *user_data);

typedef struct demo_c01_config {
    uint8_t accelerator_id;
    uint8_t brake_id;
    float stroke_mm;
    float screw_lead_mm_per_rev;
    int32_t cnt_per_motor_rev;
    int32_t standby_cnt;
    int32_t soft_limit_margin_cnt;

    int32_t move_max_rpm;
    uint8_t move_accel_100ms;
    uint8_t move_decel_100ms;
    int32_t move_tolerance_cnt;
    int32_t move_rpm_threshold;
    uint32_t move_timeout_ms;
    uint32_t poll_interval_ms;
    uint32_t post_command_delay_ms;
    int32_t max_dual_diff_cnt;
    int32_t severe_dual_diff_cnt;

    int8_t home_torque_sign;
    int32_t home_fast_current_ma;
    int32_t home_slow_current_ma;
    int16_t min_touch_current_ca;
    int16_t hard_current_limit_ca;
    int16_t load_watch_current_ca;
    int32_t pre_unwind_cnt;
    int32_t max_home_travel_cnt;
    uint32_t max_home_time_ms;
    uint32_t torque_sample_ms;
    uint32_t position_window_ms;
    int32_t stall_position_delta_cnt;
    int32_t stall_rpm;
    int32_t min_touch_travel_cnt;
    uint32_t touch_confirm_ms;
    uint8_t endgame_travel_percent;
    uint32_t torque_settle_ms;
    int32_t fast_backoff_cnt;
    int32_t fine_release_cnt;
    int32_t pre_clear_backoff_cnt;
    int16_t safe_clear_current_ca;
    int32_t final_position_tolerance_cnt;
    uint32_t pre_unwind_timeout_ms;
    uint32_t backoff_timeout_ms;
    uint32_t fine_release_timeout_ms;
    uint32_t final_release_timeout_ms;
    uint32_t inter_axis_home_settle_ms;
    uint32_t post_homing_settle_ms;

    uint32_t uart_timeout_ms;
    uint32_t inter_frame_delay_ms;
    uint32_t retry_delay_ms;
    uint8_t max_retries;

    demo_c01_rs485_direction_mode_t direction_mode;
    demo_c01_direction_callback_t set_tx_mode;
    demo_c01_direction_callback_t set_rx_mode;
    void *direction_user_data;
} demo_c01_config_t;

typedef struct demo_c01_axis_status {
    uint8_t device_id;
    uint16_t status_word;
    int16_t output_current_ca;
    int32_t motor_rpm;
    int32_t feedback_position_cnt;
    float extension_mm;
    bool is_running;
    bool has_fault;
} demo_c01_axis_status_t;

typedef struct demo_c01_dual_status {
    demo_c01_axis_status_t accelerator;
    demo_c01_axis_status_t brake;
    int32_t position_difference_cnt;
    bool difference_warning;
} demo_c01_dual_status_t;

typedef struct demo_c01_context {
    UART_Type *uart;
    demo_c01_config_t config;
    bool initialized;
    bool accelerator_homed;
    bool brake_homed;
    demo_c01_result_t last_error;
    uint8_t last_device_id;
    uint8_t last_modbus_exception;
    uint16_t last_status_word;
    int32_t last_position_cnt;
} demo_c01_context_t;

void DemoC01_GetDefaultConfig(demo_c01_config_t *config);
demo_c01_result_t DemoC01_Init(demo_c01_context_t *ctx, UART_Type *uart);
demo_c01_result_t DemoC01_InitWithConfig(demo_c01_context_t *ctx, UART_Type *uart,
                                         const demo_c01_config_t *config);
demo_c01_result_t DemoC01_Start(demo_c01_context_t *ctx, UART_Type *uart);
demo_c01_result_t DemoC01_HomeBoth(demo_c01_context_t *ctx);
demo_c01_result_t DemoC01_MoveBothCnt(demo_c01_context_t *ctx, int32_t target_cnt);
demo_c01_result_t DemoC01_MoveBothMm(demo_c01_context_t *ctx, float extension_mm);
demo_c01_result_t DemoC01_MoveBothForwardMm(demo_c01_context_t *ctx, float delta_mm);
demo_c01_result_t DemoC01_MoveBothBackwardMm(demo_c01_context_t *ctx, float delta_mm);
demo_c01_result_t DemoC01_MoveBothToStandby(demo_c01_context_t *ctx);
demo_c01_result_t DemoC01_MoveBothToFullForward(demo_c01_context_t *ctx);
demo_c01_result_t DemoC01_MoveBothToZero(demo_c01_context_t *ctx);
demo_c01_result_t DemoC01_ReadAxisStatus(demo_c01_context_t *ctx, uint8_t device_id,
                                         demo_c01_axis_status_t *status);
demo_c01_result_t DemoC01_ReadDualStatus(demo_c01_context_t *ctx,
                                         demo_c01_dual_status_t *status);
demo_c01_result_t DemoC01_BufferedStopBoth(demo_c01_context_t *ctx);
demo_c01_result_t DemoC01_EmergencyStopBoth(demo_c01_context_t *ctx);
demo_c01_result_t DemoC01_DisableBoth(demo_c01_context_t *ctx);
void DemoC01_SafeCleanup(demo_c01_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* DEMO_C_01_H */

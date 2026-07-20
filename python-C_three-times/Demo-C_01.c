#include "Demo-C_01.h"
#include "board.h"
#ifndef DEMO_C01_HOST_TEST
#include "hpm_clock_drv.h"
#include "hpm_csr_drv.h"
#endif

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint32_t demo_c01_elapsed(uint32_t start_ms, uint32_t now_ms);
static uint16_t demo_c01_crc16(const uint8_t *data, size_t len);

/*
 * HPM SDK UART 适配区。SDK 1.11 使用 uart_send_byte、uart_receive_byte、
 * uart_check_status。若后续 SDK 的 API 名称变化，只修改本区域。
 */
static void demo_c01_delay_ms(uint32_t ms)
{
    board_delay_ms(ms);
}

static uint32_t demo_c01_now_ms(void)
{
#ifdef DEMO_C01_HOST_TEST
    return board_get_ms();
#else
    uint32_t core_hz = hpm_core_clock;
    if (core_hz == 0U) {
        return 0U;
    }
    return (uint32_t)(hpm_csr_get_core_cycle() / ((uint64_t)core_hz / 1000ULL));
#endif
}

static demo_c01_result_t demo_c01_port_write(demo_c01_context_t *ctx,
                                              const uint8_t *data,
                                              size_t len)
{
    size_t i;
    if (ctx == NULL || data == NULL) {
        return DEMO_C01_ERR_NULL;
    }
    if (ctx->config.direction_mode == DEMO_C01_RS485_CALLBACK_DIRECTION) {
        ctx->config.set_tx_mode(ctx->config.direction_user_data);
    }
    for (i = 0U; i < len; ++i) {
        if (uart_send_byte(ctx->uart, data[i]) != status_success) {
            if (ctx->config.direction_mode == DEMO_C01_RS485_CALLBACK_DIRECTION) {
                ctx->config.set_rx_mode(ctx->config.direction_user_data);
            }
            return DEMO_C01_ERR_UART_TX;
        }
    }
    {
        uint32_t started = demo_c01_now_ms();
        while (!uart_check_status(ctx->uart, uart_stat_transmitter_empty)) {
            if (demo_c01_elapsed(started, demo_c01_now_ms()) >=
                ctx->config.uart_timeout_ms) {
                if (ctx->config.direction_mode == DEMO_C01_RS485_CALLBACK_DIRECTION) {
                    ctx->config.set_rx_mode(ctx->config.direction_user_data);
                }
                return DEMO_C01_ERR_UART_TIMEOUT;
            }
        }
    }
    if (ctx->config.direction_mode == DEMO_C01_RS485_CALLBACK_DIRECTION) {
        ctx->config.set_rx_mode(ctx->config.direction_user_data);
    }
    return DEMO_C01_OK;
}

static void demo_c01_flush_rx(demo_c01_context_t *ctx)
{
    uint8_t byte;
    while (uart_check_status(ctx->uart, uart_stat_data_ready)) {
        if (uart_receive_byte(ctx->uart, &byte) != status_success) {
            break;
        }
    }
}

static demo_c01_result_t demo_c01_port_read_exact(demo_c01_context_t *ctx,
                                                   uint8_t *data,
                                                   size_t len)
{
    uint32_t started;
    size_t received = 0U;
    if (ctx == NULL || data == NULL) {
        return DEMO_C01_ERR_NULL;
    }
    started = demo_c01_now_ms();
    while (received < len) {
        if (uart_check_status(ctx->uart, uart_stat_data_ready)) {
            if (uart_receive_byte(ctx->uart, &data[received]) != status_success) {
                return DEMO_C01_ERR_UART_RX;
            }
            received++;
            continue;
        }
        if (demo_c01_elapsed(started, demo_c01_now_ms()) >= ctx->config.uart_timeout_ms) {
            return DEMO_C01_ERR_UART_TIMEOUT;
        }
    }
    return DEMO_C01_OK;
}

static demo_c01_result_t demo_c01_request(demo_c01_context_t *ctx,
                                            uint8_t slave_id,
                                            uint8_t function,
                                            const uint8_t *payload,
                                            size_t payload_len,
                                            uint8_t *response,
                                            size_t response_len)
{
    uint8_t request[16];
    uint8_t attempt;
    size_t request_len;
    uint16_t crc;
    demo_c01_result_t last = DEMO_C01_ERR_UART_TIMEOUT;

    if (ctx == NULL || payload == NULL || response == NULL || payload_len > 10U ||
        response_len < 5U || slave_id == 0U || slave_id > 247U) {
        return DEMO_C01_ERR_PARAM;
    }
    ctx->last_modbus_exception = 0U;
    request[0] = slave_id;
    request[1] = function;
    memcpy(&request[2], payload, payload_len);
    request_len = payload_len + 2U;
    crc = demo_c01_crc16(request, request_len);
    request[request_len++] = (uint8_t)(crc & 0xFFU);
    request[request_len++] = (uint8_t)(crc >> 8);

    for (attempt = 0U; attempt < ctx->config.max_retries; ++attempt) {
        uint16_t got;
        demo_c01_flush_rx(ctx);
        last = demo_c01_port_write(ctx, request, request_len);
        if (last != DEMO_C01_OK) {
            return last;
        }
        last = demo_c01_port_read_exact(ctx, response, 3U);
        if (last == DEMO_C01_OK) {
            if (response[1] == (uint8_t)(function | 0x80U)) {
                last = demo_c01_port_read_exact(ctx, &response[3], 2U);
                if (last == DEMO_C01_OK) {
                    uint16_t exception_crc = (uint16_t)response[3] |
                                             ((uint16_t)response[4] << 8);
                    if (exception_crc != demo_c01_crc16(response, 3U)) {
                        last = DEMO_C01_ERR_CRC;
                    } else if (response[0] != slave_id) {
                        last = DEMO_C01_ERR_RESPONSE;
                    } else {
                        ctx->last_modbus_exception = response[2];
                        demo_c01_delay_ms(ctx->config.inter_frame_delay_ms);
                        return DEMO_C01_ERR_MODBUS_EXCEPTION;
                    }
                }
            } else {
                last = demo_c01_port_read_exact(ctx, &response[3], response_len - 3U);
            }
        }
        if (last != DEMO_C01_OK) {
            if (attempt + 1U < ctx->config.max_retries) {
                demo_c01_delay_ms(ctx->config.retry_delay_ms);
                continue;
            }
            return last;
        }
        got = (uint16_t)response[response_len - 2U] |
              ((uint16_t)response[response_len - 1U] << 8);
        if (got != demo_c01_crc16(response, response_len - 2U)) {
            last = DEMO_C01_ERR_CRC;
            if (attempt + 1U < ctx->config.max_retries) {
                demo_c01_delay_ms(ctx->config.retry_delay_ms);
                continue;
            }
            return last;
        }
        if (response[0] != slave_id) {
            return DEMO_C01_ERR_RESPONSE;
        }
        if (response[1] == (uint8_t)(function | 0x80U)) {
            ctx->last_modbus_exception = response[2];
            return DEMO_C01_ERR_MODBUS_EXCEPTION;
        }
        if (response[1] != function) {
            return DEMO_C01_ERR_RESPONSE;
        }
        demo_c01_delay_ms(ctx->config.inter_frame_delay_ms);
        return DEMO_C01_OK;
    }
    return last;
}

static demo_c01_result_t demo_c01_read_registers(demo_c01_context_t *ctx,
                                                   uint8_t slave_id,
                                                   uint16_t address,
                                                   uint16_t count,
                                                   uint16_t *values)
{
    uint8_t payload[4];
    uint8_t response[255];
    uint16_t i;
    demo_c01_result_t ret;
    if (values == NULL || count == 0U || count > 125U) {
        return DEMO_C01_ERR_PARAM;
    }
    payload[0] = (uint8_t)(address >> 8);
    payload[1] = (uint8_t)address;
    payload[2] = (uint8_t)(count >> 8);
    payload[3] = (uint8_t)count;
    ret = demo_c01_request(ctx, slave_id, 0x03U, payload, sizeof(payload),
                           response, (size_t)(5U + count * 2U));
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    if (response[2] != (uint8_t)(count * 2U)) {
        return DEMO_C01_ERR_RESPONSE;
    }
    for (i = 0U; i < count; ++i) {
        size_t offset = 3U + (size_t)i * 2U;
        values[i] = ((uint16_t)response[offset] << 8) | response[offset + 1U];
    }
    return DEMO_C01_OK;
}

static demo_c01_result_t demo_c01_write_register(demo_c01_context_t *ctx,
                                                   uint8_t slave_id,
                                                   uint16_t address,
                                                   uint16_t value)
{
    uint8_t payload[4];
    uint8_t response[8];
    demo_c01_result_t ret;
    payload[0] = (uint8_t)(address >> 8);
    payload[1] = (uint8_t)address;
    payload[2] = (uint8_t)(value >> 8);
    payload[3] = (uint8_t)value;
    ret = demo_c01_request(ctx, slave_id, 0x06U, payload, sizeof(payload), response,
                           sizeof(response));
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    return memcmp(&response[2], payload, sizeof(payload)) == 0
               ? DEMO_C01_OK : DEMO_C01_ERR_RESPONSE;
}

static demo_c01_result_t demo_c01_write_position32(demo_c01_context_t *ctx,
                                                     uint8_t slave_id,
                                                     uint16_t address,
                                                     int32_t value)
{
    uint32_t raw = (uint32_t)value;
    uint8_t payload[9];
    uint8_t response[8];
    demo_c01_result_t ret;
    payload[0] = (uint8_t)(address >> 8);
    payload[1] = (uint8_t)address;
    payload[2] = 0U;
    payload[3] = 2U;
    payload[4] = 4U;
    payload[5] = (uint8_t)(raw >> 24);
    payload[6] = (uint8_t)(raw >> 16);
    payload[7] = (uint8_t)(raw >> 8);
    payload[8] = (uint8_t)raw;
    ret = demo_c01_request(ctx, slave_id, 0x10U, payload, sizeof(payload), response,
                           sizeof(response));
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    if (response[2] != payload[0] || response[3] != payload[1] ||
        response[4] != 0U || response[5] != 2U) {
        return DEMO_C01_ERR_RESPONSE;
    }
    return DEMO_C01_OK;
}

static uint32_t demo_c01_elapsed(uint32_t start_ms, uint32_t now_ms)
{
    return now_ms - start_ms;
}

static uint16_t demo_c01_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFU;
    size_t i;

    for (i = 0U; i < len; ++i) {
        uint8_t bit;
        crc ^= data[i];
        for (bit = 0U; bit < 8U; ++bit) {
            crc = ((crc & 1U) != 0U) ? (uint16_t)((crc >> 1) ^ 0xA001U)
                                     : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

static uint16_t demo_c01_rpm_to_units(int32_t rpm)
{
    int64_t value = ((int64_t)rpm * 8192LL);
    value = value >= 0 ? (value + 1500LL) / 3000LL : -((-value + 1500LL) / 3000LL);
    if (value > INT16_MAX) {
        value = INT16_MAX;
    } else if (value < INT16_MIN) {
        value = INT16_MIN;
    }
    return (uint16_t)(int16_t)value;
}

static int32_t demo_c01_units_to_rpm(uint16_t raw)
{
    int32_t units = (int32_t)(int16_t)raw;
    int32_t magnitude = units < 0 ? -units : units;
    int32_t rpm = (magnitude * 3000 + 4096) / 8192;
    return units < 0 ? -rpm : rpm;
}

static double demo_c01_cnt_per_mm(const demo_c01_config_t *cfg)
{
    return (double)cfg->cnt_per_motor_rev / (double)cfg->screw_lead_mm_per_rev;
}

static demo_c01_result_t demo_c01_mm_to_cnt(const demo_c01_config_t *cfg,
                                             float mm,
                                             int32_t *cnt)
{
    double raw;

    if (cfg == NULL || cnt == NULL || !isfinite(mm) || mm < 0.0f) {
        return DEMO_C01_ERR_PARAM;
    }
    if (mm > cfg->stroke_mm) {
        return DEMO_C01_ERR_SOFT_LIMIT;
    }
    raw = -(double)mm * demo_c01_cnt_per_mm(cfg);
    if (!isfinite(raw) || raw > (double)INT32_MAX || raw < (double)INT32_MIN) {
        return DEMO_C01_ERR_SOFT_LIMIT;
    }
    *cnt = (int32_t)(raw >= 0.0 ? raw + 0.5 : raw - 0.5);
    return DEMO_C01_OK;
}

static float demo_c01_cnt_to_mm(const demo_c01_config_t *cfg, int32_t cnt)
{
    return (float)(-(double)cnt / demo_c01_cnt_per_mm(cfg));
}

static demo_c01_result_t demo_c01_validate_config(const demo_c01_config_t *cfg)
{
    if (cfg == NULL) {
        return DEMO_C01_ERR_NULL;
    }
    if (cfg->accelerator_id == 0U || cfg->accelerator_id > 247U ||
        cfg->brake_id == 0U || cfg->brake_id > 247U ||
        cfg->accelerator_id == cfg->brake_id ||
        !isfinite(cfg->stroke_mm) || !isfinite(cfg->screw_lead_mm_per_rev) ||
        cfg->stroke_mm <= 0.0f || cfg->screw_lead_mm_per_rev <= 0.0f ||
        cfg->cnt_per_motor_rev <= 0 || cfg->move_max_rpm <= 0 ||
        cfg->move_max_rpm > 3000 || cfg->move_tolerance_cnt < 0 ||
        cfg->move_timeout_ms == 0U || cfg->poll_interval_ms == 0U ||
        (cfg->home_torque_sign != 1 && cfg->home_torque_sign != -1) ||
        cfg->home_fast_current_ma <= 0 ||
        cfg->home_slow_current_ma <= 0 ||
        cfg->home_fast_current_ma < cfg->home_slow_current_ma ||
        cfg->home_fast_current_ma > INT16_MAX ||
        cfg->home_slow_current_ma > INT16_MAX ||
        cfg->min_touch_current_ca <= 0 ||
        cfg->hard_current_limit_ca < cfg->min_touch_current_ca ||
        cfg->max_home_time_ms == 0U || cfg->max_home_travel_cnt <= 0 ||
        cfg->torque_sample_ms == 0U || cfg->position_window_ms == 0U ||
        cfg->endgame_travel_percent == 0U || cfg->endgame_travel_percent > 100U ||
        cfg->soft_limit_margin_cnt < 0 || cfg->max_dual_diff_cnt < 0 ||
        cfg->severe_dual_diff_cnt <= cfg->max_dual_diff_cnt ||
        cfg->fast_backoff_cnt == INT32_MIN || cfg->fine_release_cnt == INT32_MIN ||
        cfg->pre_clear_backoff_cnt == INT32_MIN || cfg->max_retries == 0U) {
        return DEMO_C01_ERR_PARAM;
    }
    if (cfg->direction_mode == DEMO_C01_RS485_CALLBACK_DIRECTION &&
        (cfg->set_tx_mode == NULL || cfg->set_rx_mode == NULL)) {
        return DEMO_C01_ERR_PARAM;
    }
    return DEMO_C01_OK;
}

void DemoC01_GetDefaultConfig(demo_c01_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->accelerator_id = 1U;
    cfg->brake_id = 2U;
    cfg->stroke_mm = 300.0f;
    cfg->screw_lead_mm_per_rev = 5.0f;
    cfg->cnt_per_motor_rev = 10000;
    cfg->standby_cnt = -500;
    cfg->soft_limit_margin_cnt = 1000000;
    cfg->move_max_rpm = 300;
    cfg->move_accel_100ms = 2U;
    cfg->move_decel_100ms = 2U;
    cfg->move_tolerance_cnt = 80;
    cfg->move_rpm_threshold = 5;
    cfg->move_timeout_ms = 12000U;
    cfg->poll_interval_ms = 80U;
    cfg->post_command_delay_ms = 80U;
    cfg->max_dual_diff_cnt = 200;
    cfg->severe_dual_diff_cnt = 20000;
    cfg->home_torque_sign = 1;
    cfg->home_fast_current_ma = 1500;
    cfg->home_slow_current_ma = 1300;
    cfg->min_touch_current_ca = 55;
    cfg->hard_current_limit_ca = 230;
    cfg->load_watch_current_ca = 140;
    cfg->pre_unwind_cnt = -2500;
    cfg->max_home_travel_cnt = 763500;
    cfg->max_home_time_ms = 60000U;
    cfg->torque_sample_ms = 50U;
    cfg->position_window_ms = 200U;
    cfg->stall_position_delta_cnt = 8;
    cfg->stall_rpm = 5;
    cfg->min_touch_travel_cnt = 200;
    cfg->touch_confirm_ms = 500U;
    cfg->endgame_travel_percent = 65U;
    cfg->torque_settle_ms = 200U;
    cfg->fast_backoff_cnt = -500;
    cfg->fine_release_cnt = -100;
    cfg->pre_clear_backoff_cnt = -500;
    cfg->safe_clear_current_ca = 40;
    cfg->final_position_tolerance_cnt = 120;
    cfg->pre_unwind_timeout_ms = 3000U;
    cfg->backoff_timeout_ms = 2000U;
    cfg->fine_release_timeout_ms = 1200U;
    cfg->final_release_timeout_ms = 1500U;
    cfg->inter_axis_home_settle_ms = 350U;
    cfg->post_homing_settle_ms = 250U;
    cfg->uart_timeout_ms = 500U;
    cfg->inter_frame_delay_ms = 4U;
    cfg->retry_delay_ms = 60U;
    cfg->max_retries = 3U;
    cfg->direction_mode = DEMO_C01_RS485_AUTO_DIRECTION;
}

demo_c01_result_t DemoC01_InitWithConfig(demo_c01_context_t *ctx,
                                         UART_Type *uart,
                                         const demo_c01_config_t *cfg)
{
    demo_c01_result_t ret;
    if (ctx == NULL || uart == NULL || cfg == NULL) {
        return DEMO_C01_ERR_NULL;
    }
#ifndef DEMO_C01_HOST_TEST
    if (hpm_core_clock < 1000U) {
        return DEMO_C01_ERR_SDK_ADAPTATION;
    }
#endif
    ret = demo_c01_validate_config(cfg);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->uart = uart;
    ctx->config = *cfg;
    ctx->initialized = true;
    ctx->last_error = DEMO_C01_OK;
    return DEMO_C01_OK;
}

demo_c01_result_t DemoC01_Init(demo_c01_context_t *ctx, UART_Type *uart)
{
    demo_c01_config_t cfg;
    DemoC01_GetDefaultConfig(&cfg);
    return DemoC01_InitWithConfig(ctx, uart, &cfg);
}

/* IDS830ABS RS485 registers. */
#define DEMO_C01_REG_ENABLE          0x00U
#define DEMO_C01_REG_MODE_SELECT     0x02U
#define DEMO_C01_REG_TORQUE_TARGET   0x08U
#define DEMO_C01_REG_POS_ACCEL       0x09U
#define DEMO_C01_REG_COMM_TIMEOUT    0x1CU
#define DEMO_C01_REG_MAX_VEL_LIMIT   0x1DU
#define DEMO_C01_REG_CTRL_STATE      0x36U
#define DEMO_C01_REG_CLEAR_FAULT     0x4AU
#define DEMO_C01_REG_CLEAR_POSITION  0x4CU
#define DEMO_C01_REG_EMERGENCY_STOP  0x4DU
#define DEMO_C01_REG_BUFFERED_STOP   0x4FU
#define DEMO_C01_REG_POSITION_HIGH   0x50U
#define DEMO_C01_REG_ABS_REL         0x51U
#define DEMO_C01_REG_FAST_MONITOR    0xE3U
#define DEMO_C01_MODE_TORQUE_PC      0x00C1U
#define DEMO_C01_MODE_POSITION_PC    0x00D0U
#define DEMO_C01_CTRL_STATE_PC       0x0002U

static demo_c01_result_t demo_c01_record_error(demo_c01_context_t *ctx,
                                                uint8_t id,
                                                demo_c01_result_t error)
{
    if (ctx != NULL && error != DEMO_C01_OK) {
        ctx->last_error = error;
        ctx->last_device_id = id;
    }
    return error;
}

static int64_t demo_c01_abs_diff(int32_t a, int32_t b)
{
    int64_t value = (int64_t)a - (int64_t)b;
    return value < 0 ? -value : value;
}

static int32_t demo_c01_abs_i16(int16_t value)
{
    return value < 0 ? -(int32_t)value : (int32_t)value;
}

static bool demo_c01_status_has_fault(uint16_t status_word)
{
    return (status_word & 0x007EU) != 0U;
}

static demo_c01_result_t demo_c01_axis_enable(demo_c01_context_t *ctx,
                                               uint8_t id,
                                               bool enabled)
{
    return demo_c01_write_register(ctx, id, DEMO_C01_REG_ENABLE,
                                    enabled ? 1U : 0U);
}

static demo_c01_result_t demo_c01_axis_clear_fault(demo_c01_context_t *ctx,
                                                    uint8_t id)
{
    return demo_c01_write_register(ctx, id, DEMO_C01_REG_CLEAR_FAULT, 0U);
}

static demo_c01_result_t demo_c01_axis_buffered_stop(demo_c01_context_t *ctx,
                                                      uint8_t id)
{
    return demo_c01_write_register(ctx, id, DEMO_C01_REG_BUFFERED_STOP, 0U);
}

static demo_c01_result_t demo_c01_axis_emergency_stop(demo_c01_context_t *ctx,
                                                       uint8_t id)
{
    return demo_c01_write_register(ctx, id, DEMO_C01_REG_EMERGENCY_STOP, 0U);
}

static demo_c01_result_t demo_c01_axis_standard_init(demo_c01_context_t *ctx,
                                                      uint8_t id)
{
    demo_c01_result_t ret = demo_c01_axis_clear_fault(ctx, id);
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_write_register(ctx, id, DEMO_C01_REG_CTRL_STATE,
                                      DEMO_C01_CTRL_STATE_PC);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_write_register(ctx, id, DEMO_C01_REG_COMM_TIMEOUT, 7U);
    }
    return demo_c01_record_error(ctx, id, ret);
}

static demo_c01_result_t demo_c01_axis_prepare_torque(demo_c01_context_t *ctx,
                                                       uint8_t id)
{
    demo_c01_result_t ret = demo_c01_write_register(ctx, id,
                                                     DEMO_C01_REG_MODE_SELECT,
                                                     DEMO_C01_MODE_TORQUE_PC);
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_enable(ctx, id, true);
    }
    return demo_c01_record_error(ctx, id, ret);
}

static demo_c01_result_t demo_c01_axis_set_current_ma(demo_c01_context_t *ctx,
                                                       uint8_t id,
                                                       int32_t current_ma)
{
    if (current_ma > INT16_MAX || current_ma < INT16_MIN) {
        return demo_c01_record_error(ctx, id, DEMO_C01_ERR_PARAM);
    }
    return demo_c01_record_error(ctx, id,
        demo_c01_write_register(ctx, id, DEMO_C01_REG_TORQUE_TARGET,
                                (uint16_t)(int16_t)current_ma));
}

static demo_c01_result_t demo_c01_axis_prepare_position(demo_c01_context_t *ctx,
                                                         uint8_t id,
                                                         int32_t max_rpm,
                                                         bool absolute)
{
    demo_c01_result_t ret;
    if (max_rpm <= 0 || max_rpm > 3000) {
        return DEMO_C01_ERR_PARAM;
    }
    ret = demo_c01_write_register(ctx, id, DEMO_C01_REG_MODE_SELECT,
                                  DEMO_C01_MODE_POSITION_PC);
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_write_register(ctx, id, DEMO_C01_REG_POS_ACCEL,
            (uint16_t)(((uint16_t)ctx->config.move_accel_100ms << 8) |
                       ctx->config.move_decel_100ms));
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_write_register(ctx, id, DEMO_C01_REG_MAX_VEL_LIMIT,
                                      demo_c01_rpm_to_units(max_rpm));
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_write_register(ctx, id, DEMO_C01_REG_ABS_REL,
                                      absolute ? 0U : 1U);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_enable(ctx, id, true);
    }
    return demo_c01_record_error(ctx, id, ret);
}

static demo_c01_result_t demo_c01_axis_set_position(demo_c01_context_t *ctx,
                                                     uint8_t id,
                                                     int32_t target_cnt)
{
    return demo_c01_record_error(ctx, id,
        demo_c01_write_position32(ctx, id, DEMO_C01_REG_POSITION_HIGH,
                                  target_cnt));
}

static demo_c01_result_t demo_c01_axis_read_fast_status(
    demo_c01_context_t *ctx,
    uint8_t id,
    demo_c01_axis_status_t *status)
{
    uint16_t regs[5];
    demo_c01_result_t ret;
    if (status == NULL) {
        return DEMO_C01_ERR_NULL;
    }
    ret = demo_c01_read_registers(ctx, id, DEMO_C01_REG_FAST_MONITOR, 5U, regs);
    if (ret != DEMO_C01_OK) {
        return demo_c01_record_error(ctx, id, ret);
    }
    status->device_id = id;
    status->status_word = regs[0];
    status->output_current_ca = (int16_t)regs[1];
    status->motor_rpm = demo_c01_units_to_rpm(regs[2]);
    status->feedback_position_cnt = (int32_t)(((uint32_t)regs[3] << 16) | regs[4]);
    status->extension_mm = demo_c01_cnt_to_mm(&ctx->config,
                                               status->feedback_position_cnt);
    status->is_running = (regs[0] & 1U) != 0U;
    status->has_fault = demo_c01_status_has_fault(regs[0]);
    ctx->last_device_id = id;
    ctx->last_status_word = regs[0];
    ctx->last_position_cnt = status->feedback_position_cnt;
    if (status->has_fault) {
        return demo_c01_record_error(ctx, id, DEMO_C01_ERR_SERVO_FAULT);
    }
    return DEMO_C01_OK;
}

static demo_c01_result_t demo_c01_wait_axis_settled(demo_c01_context_t *ctx,
                                                     uint8_t id,
                                                     int32_t target_cnt,
                                                     uint32_t timeout_ms,
                                                     int32_t tolerance_cnt)
{
    uint32_t started = demo_c01_now_ms();
    do {
        demo_c01_axis_status_t status;
        demo_c01_result_t ret = demo_c01_axis_read_fast_status(ctx, id, &status);
        if (ret != DEMO_C01_OK) {
            return ret;
        }
        if (demo_c01_abs_diff(status.feedback_position_cnt, target_cnt) <= tolerance_cnt &&
            status.motor_rpm >= -ctx->config.move_rpm_threshold &&
            status.motor_rpm <= ctx->config.move_rpm_threshold) {
            return DEMO_C01_OK;
        }
        demo_c01_delay_ms(ctx->config.poll_interval_ms);
    } while (demo_c01_elapsed(started, demo_c01_now_ms()) < timeout_ms);
    return demo_c01_record_error(ctx, id, DEMO_C01_ERR_MOVE_TIMEOUT);
}

static demo_c01_result_t demo_c01_move_abs(demo_c01_context_t *ctx,
                                            uint8_t id,
                                            int32_t target_cnt,
                                            int32_t max_rpm,
                                            uint32_t timeout_ms,
                                            int32_t tolerance_cnt)
{
    demo_c01_result_t ret = demo_c01_axis_prepare_position(ctx, id, max_rpm, true);
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_set_position(ctx, id, target_cnt);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_wait_axis_settled(ctx, id, target_cnt, timeout_ms,
                                         tolerance_cnt);
    }
    return ret;
}

static int32_t demo_c01_release_offset(const demo_c01_context_t *ctx,
                                       int32_t configured)
{
    int32_t magnitude = configured < 0 ? -configured : configured;
    return ctx->config.home_torque_sign >= 0 ? -magnitude : magnitude;
}

static demo_c01_result_t demo_c01_probe_by_torque(demo_c01_context_t *ctx,
                                                   uint8_t id,
                                                   int32_t current_ma,
                                                   int32_t *touch_position)
{
    demo_c01_axis_status_t initial;
    int32_t window_position;
    uint32_t started;
    uint32_t window_started;
    uint32_t confirm_started = 0U;
    bool confirming = false;
    bool slowed = false;
    demo_c01_result_t ret;

    ret = demo_c01_axis_prepare_torque(ctx, id);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    ret = demo_c01_axis_set_current_ma(ctx, id,
        ctx->config.home_torque_sign * current_ma);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    ret = demo_c01_axis_read_fast_status(ctx, id, &initial);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    window_position = initial.feedback_position_cnt;
    started = demo_c01_now_ms();
    window_started = started;

    while (demo_c01_elapsed(started, demo_c01_now_ms()) <
           ctx->config.max_home_time_ms) {
        demo_c01_axis_status_t status;
        int64_t travel;
        int64_t position_delta;
        uint32_t now;
        bool stalled;

        ret = demo_c01_axis_read_fast_status(ctx, id, &status);
        if (ret != DEMO_C01_OK) {
            break;
        }
        now = demo_c01_now_ms();
        travel = ctx->config.home_torque_sign >= 0
                   ? (int64_t)status.feedback_position_cnt - initial.feedback_position_cnt
                   : (int64_t)initial.feedback_position_cnt - status.feedback_position_cnt;
        if (travel > ctx->config.max_home_travel_cnt) {
            ret = DEMO_C01_ERR_HOMING_TRAVEL;
            break;
        }
        if (demo_c01_abs_i16(status.output_current_ca) >=
            ctx->config.hard_current_limit_ca) {
            if (travel < ctx->config.min_touch_travel_cnt) {
                ret = travel <= 0 ? DEMO_C01_ERR_HOMING_TRAVEL
                                  : DEMO_C01_ERR_HOMING_CURRENT;
                break;
            }
            ret = DEMO_C01_OK;
            if (touch_position != NULL) {
                *touch_position = status.feedback_position_cnt;
            }
            goto probe_stop;
        }
        if (!slowed && travel >= ctx->config.min_touch_travel_cnt &&
            (demo_c01_abs_i16(status.output_current_ca) >=
                 ctx->config.load_watch_current_ca ||
             travel * 100 >= (int64_t)ctx->config.max_home_travel_cnt *
                                  ctx->config.endgame_travel_percent)) {
            ret = demo_c01_axis_set_current_ma(ctx, id,
                ctx->config.home_torque_sign * ctx->config.home_slow_current_ma);
            if (ret != DEMO_C01_OK) {
                break;
            }
            slowed = true;
        }
        position_delta = demo_c01_abs_diff(status.feedback_position_cnt,
                                            window_position);
        stalled = travel >= ctx->config.min_touch_travel_cnt &&
                  position_delta <= ctx->config.stall_position_delta_cnt &&
                  status.motor_rpm >= -ctx->config.stall_rpm &&
                  status.motor_rpm <= ctx->config.stall_rpm &&
                  demo_c01_abs_i16(status.output_current_ca) >=
                      ctx->config.min_touch_current_ca;
        if (stalled) {
            if (!confirming) {
                confirming = true;
                confirm_started = now;
            } else if (demo_c01_elapsed(confirm_started, now) >=
                       ctx->config.touch_confirm_ms) {
                if (touch_position != NULL) {
                    *touch_position = status.feedback_position_cnt;
                }
                ret = DEMO_C01_OK;
                goto probe_stop;
            }
        } else {
            confirming = false;
        }
        if (demo_c01_elapsed(window_started, now) >= ctx->config.position_window_ms) {
            window_position = status.feedback_position_cnt;
            window_started = now;
        }
        demo_c01_delay_ms(ctx->config.torque_sample_ms);
    }
    if (ret == DEMO_C01_OK) {
        ret = DEMO_C01_ERR_HOMING_TIMEOUT;
    }

probe_stop:
    {
        demo_c01_result_t stop1 = demo_c01_axis_set_current_ma(ctx, id, 0);
        demo_c01_result_t stop2 = demo_c01_axis_buffered_stop(ctx, id);
        if (ret == DEMO_C01_OK && (stop1 != DEMO_C01_OK || stop2 != DEMO_C01_OK)) {
            ret = DEMO_C01_ERR_HOMING_CURRENT;
        }
    }
    demo_c01_delay_ms(ctx->config.torque_settle_ms);
    return demo_c01_record_error(ctx, id, ret);
}

static demo_c01_result_t demo_c01_wait_current_below(demo_c01_context_t *ctx,
                                                      uint8_t id,
                                                      uint32_t timeout_ms)
{
    uint32_t started = demo_c01_now_ms();
    do {
        demo_c01_axis_status_t status;
        demo_c01_result_t ret = demo_c01_axis_read_fast_status(ctx, id, &status);
        if (ret != DEMO_C01_OK) {
            return ret;
        }
        if (demo_c01_abs_i16(status.output_current_ca) <=
            ctx->config.safe_clear_current_ca) {
            return DEMO_C01_OK;
        }
        demo_c01_delay_ms(ctx->config.torque_sample_ms);
    } while (demo_c01_elapsed(started, demo_c01_now_ms()) < timeout_ms);
    return demo_c01_record_error(ctx, id, DEMO_C01_ERR_HOMING_CURRENT);
}

static demo_c01_result_t demo_c01_home_one_axis(demo_c01_context_t *ctx,
                                                 uint8_t id)
{
    demo_c01_axis_status_t before;
    int32_t touch;
    int32_t target;
    demo_c01_result_t ret = demo_c01_axis_clear_fault(ctx, id);
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_buffered_stop(ctx, id);
    }
    demo_c01_delay_ms(100U);
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_read_fast_status(ctx, id, &before);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_standard_init(ctx, id);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_prepare_position(ctx, id, 60, false);
    }
    if (ret == DEMO_C01_OK) {
        target = before.feedback_position_cnt + ctx->config.pre_unwind_cnt;
        ret = demo_c01_axis_set_position(ctx, id, ctx->config.pre_unwind_cnt);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_wait_axis_settled(ctx, id, target,
            ctx->config.pre_unwind_timeout_ms, ctx->config.move_tolerance_cnt);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_probe_by_torque(ctx, id,
                                       ctx->config.home_fast_current_ma, &touch);
    }
    if (ret == DEMO_C01_OK) {
        target = touch + demo_c01_release_offset(ctx, ctx->config.fast_backoff_cnt);
        ret = demo_c01_move_abs(ctx, id, target, 40,
                                ctx->config.backoff_timeout_ms,
                                ctx->config.move_tolerance_cnt);
    }
    if (ret == DEMO_C01_OK && ctx->config.fine_release_cnt != 0) {
        target += demo_c01_release_offset(ctx, ctx->config.fine_release_cnt);
        ret = demo_c01_move_abs(ctx, id, target, 35,
                                ctx->config.fine_release_timeout_ms,
                                ctx->config.move_tolerance_cnt);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_probe_by_torque(ctx, id,
                                       ctx->config.home_slow_current_ma, &touch);
    }
    if (ret == DEMO_C01_OK) {
        target = touch + demo_c01_release_offset(ctx,
                                                 ctx->config.pre_clear_backoff_cnt);
        ret = demo_c01_move_abs(ctx, id, target, 40,
                                ctx->config.backoff_timeout_ms,
                                ctx->config.move_tolerance_cnt);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_wait_current_below(ctx, id, 3000U);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_write_register(ctx, id, DEMO_C01_REG_CLEAR_POSITION, 0U);
        demo_c01_delay_ms(ctx->config.torque_settle_ms);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_move_abs(ctx, id, ctx->config.standby_cnt, 50,
                                ctx->config.final_release_timeout_ms,
                                ctx->config.final_position_tolerance_cnt);
    }
    return demo_c01_record_error(ctx, id, ret);
}

static demo_c01_result_t demo_c01_require_initialized(demo_c01_context_t *ctx)
{
    if (ctx == NULL) {
        return DEMO_C01_ERR_NULL;
    }
    return ctx->initialized ? DEMO_C01_OK : DEMO_C01_ERR_NOT_INITIALIZED;
}

static demo_c01_result_t demo_c01_check_soft_limit(demo_c01_context_t *ctx,
                                                    int32_t target_cnt)
{
    int32_t full_forward;
    demo_c01_result_t ret = demo_c01_mm_to_cnt(&ctx->config,
                                                ctx->config.stroke_mm,
                                                &full_forward);
    int64_t lower;
    int64_t upper;
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    lower = (int64_t)full_forward - ctx->config.soft_limit_margin_cnt;
    upper = (int64_t)ctx->config.soft_limit_margin_cnt;
    return (int64_t)target_cnt < lower || (int64_t)target_cnt > upper
             ? DEMO_C01_ERR_SOFT_LIMIT : DEMO_C01_OK;
}

static demo_c01_result_t demo_c01_wait_both(demo_c01_context_t *ctx,
                                             int32_t target_cnt)
{
    uint32_t started = demo_c01_now_ms();
    do {
        demo_c01_axis_status_t accelerator;
        demo_c01_axis_status_t brake;
        demo_c01_result_t ret = demo_c01_axis_read_fast_status(
            ctx, ctx->config.accelerator_id, &accelerator);
        if (ret != DEMO_C01_OK) {
            return ret;
        }
        demo_c01_delay_ms(20U);
        ret = demo_c01_axis_read_fast_status(ctx, ctx->config.brake_id, &brake);
        if (ret != DEMO_C01_OK) {
            return ret;
        }
        if (demo_c01_abs_diff(accelerator.feedback_position_cnt,
                              brake.feedback_position_cnt) >=
            ctx->config.severe_dual_diff_cnt) {
            (void)DemoC01_BufferedStopBoth(ctx);
            return demo_c01_record_error(ctx, 0U, DEMO_C01_ERR_DUAL_DIFFERENCE);
        }
        if (demo_c01_abs_diff(accelerator.feedback_position_cnt, target_cnt) <=
                ctx->config.move_tolerance_cnt &&
            demo_c01_abs_diff(brake.feedback_position_cnt, target_cnt) <=
                ctx->config.move_tolerance_cnt &&
            accelerator.motor_rpm >= -ctx->config.move_rpm_threshold &&
            accelerator.motor_rpm <= ctx->config.move_rpm_threshold &&
            brake.motor_rpm >= -ctx->config.move_rpm_threshold &&
            brake.motor_rpm <= ctx->config.move_rpm_threshold) {
            if (demo_c01_abs_diff(accelerator.feedback_position_cnt,
                                  brake.feedback_position_cnt) >
                ctx->config.max_dual_diff_cnt) {
                return demo_c01_record_error(ctx, 0U,
                                              DEMO_C01_ERR_DUAL_DIFFERENCE);
            }
            return DEMO_C01_OK;
        }
        demo_c01_delay_ms(ctx->config.poll_interval_ms);
    } while (demo_c01_elapsed(started, demo_c01_now_ms()) <
             ctx->config.move_timeout_ms);
    return demo_c01_record_error(ctx, 0U, DEMO_C01_ERR_MOVE_TIMEOUT);
}

demo_c01_result_t DemoC01_HomeBoth(demo_c01_context_t *ctx)
{
    demo_c01_result_t ret = demo_c01_require_initialized(ctx);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    ctx->accelerator_homed = false;
    ctx->brake_homed = false;
    ret = demo_c01_home_one_axis(ctx, ctx->config.accelerator_id);
    if (ret == DEMO_C01_OK) {
        ctx->accelerator_homed = true;
        demo_c01_delay_ms(ctx->config.inter_axis_home_settle_ms);
        ret = demo_c01_home_one_axis(ctx, ctx->config.brake_id);
    }
    if (ret == DEMO_C01_OK) {
        ctx->brake_homed = true;
        demo_c01_delay_ms(ctx->config.post_homing_settle_ms);
        return DEMO_C01_OK;
    }
    ctx->accelerator_homed = false;
    ctx->brake_homed = false;
    DemoC01_SafeCleanup(ctx);
    return ret;
}

demo_c01_result_t DemoC01_MoveBothCnt(demo_c01_context_t *ctx,
                                       int32_t target_cnt)
{
    demo_c01_result_t ret = demo_c01_require_initialized(ctx);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    if (!ctx->accelerator_homed || !ctx->brake_homed) {
        return demo_c01_record_error(ctx, 0U, DEMO_C01_ERR_NOT_HOMED);
    }
    ret = demo_c01_check_soft_limit(ctx, target_cnt);
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_prepare_position(ctx, ctx->config.accelerator_id,
                                              ctx->config.move_max_rpm, true);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_prepare_position(ctx, ctx->config.brake_id,
                                              ctx->config.move_max_rpm, true);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_axis_set_position(ctx, ctx->config.accelerator_id,
                                          target_cnt);
    }
    if (ret == DEMO_C01_OK) {
        demo_c01_delay_ms(ctx->config.post_command_delay_ms);
        ret = demo_c01_axis_set_position(ctx, ctx->config.brake_id, target_cnt);
    }
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_wait_both(ctx, target_cnt);
    }
    if (ret != DEMO_C01_OK) {
        DemoC01_SafeCleanup(ctx);
    }
    return ret;
}

demo_c01_result_t DemoC01_MoveBothMm(demo_c01_context_t *ctx,
                                      float extension_mm)
{
    int32_t target;
    demo_c01_result_t ret = demo_c01_require_initialized(ctx);
    if (ret == DEMO_C01_OK) {
        ret = demo_c01_mm_to_cnt(&ctx->config, extension_mm, &target);
    }
    return ret == DEMO_C01_OK ? DemoC01_MoveBothCnt(ctx, target) : ret;
}

static demo_c01_result_t demo_c01_move_relative_mm(demo_c01_context_t *ctx,
                                                    float delta_mm,
                                                    bool forward)
{
    demo_c01_dual_status_t status;
    int32_t delta;
    int64_t average;
    int64_t target;
    demo_c01_result_t ret;
    if (delta_mm < 0.0f || !isfinite(delta_mm)) {
        return DEMO_C01_ERR_PARAM;
    }
    ret = DemoC01_ReadDualStatus(ctx, &status);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    ret = demo_c01_mm_to_cnt(&ctx->config, delta_mm, &delta);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    if (delta < 0) {
        delta = -delta;
    }
    average = ((int64_t)status.accelerator.feedback_position_cnt +
               status.brake.feedback_position_cnt) / 2;
    target = forward ? average - delta : average + delta;
    if (target < INT32_MIN || target > INT32_MAX) {
        return DEMO_C01_ERR_SOFT_LIMIT;
    }
    return DemoC01_MoveBothCnt(ctx, (int32_t)target);
}

demo_c01_result_t DemoC01_MoveBothForwardMm(demo_c01_context_t *ctx,
                                             float delta_mm)
{
    return demo_c01_move_relative_mm(ctx, delta_mm, true);
}

demo_c01_result_t DemoC01_MoveBothBackwardMm(demo_c01_context_t *ctx,
                                              float delta_mm)
{
    return demo_c01_move_relative_mm(ctx, delta_mm, false);
}

demo_c01_result_t DemoC01_MoveBothToStandby(demo_c01_context_t *ctx)
{
    return DemoC01_MoveBothCnt(ctx, ctx != NULL ? ctx->config.standby_cnt : 0);
}

demo_c01_result_t DemoC01_MoveBothToFullForward(demo_c01_context_t *ctx)
{
    return ctx == NULL ? DEMO_C01_ERR_NULL
                       : DemoC01_MoveBothMm(ctx, ctx->config.stroke_mm);
}

demo_c01_result_t DemoC01_MoveBothToZero(demo_c01_context_t *ctx)
{
    return DemoC01_MoveBothCnt(ctx, 0);
}

demo_c01_result_t DemoC01_ReadAxisStatus(demo_c01_context_t *ctx,
                                          uint8_t device_id,
                                          demo_c01_axis_status_t *status)
{
    demo_c01_result_t ret = demo_c01_require_initialized(ctx);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    if (status == NULL) {
        return DEMO_C01_ERR_NULL;
    }
    if (device_id != ctx->config.accelerator_id &&
        device_id != ctx->config.brake_id) {
        return DEMO_C01_ERR_PARAM;
    }
    return demo_c01_axis_read_fast_status(ctx, device_id, status);
}

demo_c01_result_t DemoC01_ReadDualStatus(demo_c01_context_t *ctx,
                                          demo_c01_dual_status_t *status)
{
    int64_t difference;
    demo_c01_result_t ret = demo_c01_require_initialized(ctx);
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    if (status == NULL) {
        return DEMO_C01_ERR_NULL;
    }
    ret = DemoC01_ReadAxisStatus(ctx, ctx->config.accelerator_id,
                                 &status->accelerator);
    if (ret == DEMO_C01_OK) {
        ret = DemoC01_ReadAxisStatus(ctx, ctx->config.brake_id, &status->brake);
    }
    if (ret != DEMO_C01_OK) {
        return ret;
    }
    difference = demo_c01_abs_diff(status->accelerator.feedback_position_cnt,
                                   status->brake.feedback_position_cnt);
    status->position_difference_cnt = difference > INT32_MAX
                                        ? INT32_MAX : (int32_t)difference;
    status->difference_warning = difference > ctx->config.max_dual_diff_cnt;
    return DEMO_C01_OK;
}

static demo_c01_result_t demo_c01_apply_both(demo_c01_context_t *ctx,
    demo_c01_result_t (*operation)(demo_c01_context_t *, uint8_t))
{
    demo_c01_result_t first;
    demo_c01_result_t second;
    demo_c01_result_t ready = demo_c01_require_initialized(ctx);
    if (ready != DEMO_C01_OK) {
        return ready;
    }
    first = operation(ctx, ctx->config.accelerator_id);
    second = operation(ctx, ctx->config.brake_id);
    return first != DEMO_C01_OK ? first : second;
}

static demo_c01_result_t demo_c01_disable_axis(demo_c01_context_t *ctx,
                                                uint8_t id)
{
    return demo_c01_axis_enable(ctx, id, false);
}

demo_c01_result_t DemoC01_BufferedStopBoth(demo_c01_context_t *ctx)
{
    return demo_c01_apply_both(ctx, demo_c01_axis_buffered_stop);
}

demo_c01_result_t DemoC01_EmergencyStopBoth(demo_c01_context_t *ctx)
{
    return demo_c01_apply_both(ctx, demo_c01_axis_emergency_stop);
}

demo_c01_result_t DemoC01_DisableBoth(demo_c01_context_t *ctx)
{
    return demo_c01_apply_both(ctx, demo_c01_disable_axis);
}

void DemoC01_SafeCleanup(demo_c01_context_t *ctx)
{
    demo_c01_result_t original;
    uint8_t original_device;
    uint8_t original_exception;
    uint16_t original_status;
    int32_t original_position;
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    original = ctx->last_error;
    original_device = ctx->last_device_id;
    original_exception = ctx->last_modbus_exception;
    original_status = ctx->last_status_word;
    original_position = ctx->last_position_cnt;
    (void)demo_c01_axis_set_current_ma(ctx, ctx->config.accelerator_id, 0);
    (void)demo_c01_axis_set_current_ma(ctx, ctx->config.brake_id, 0);
    (void)DemoC01_BufferedStopBoth(ctx);
    (void)DemoC01_DisableBoth(ctx);
    if (original != DEMO_C01_OK) {
        ctx->last_error = original;
        ctx->last_device_id = original_device;
        ctx->last_modbus_exception = original_exception;
        ctx->last_status_word = original_status;
        ctx->last_position_cnt = original_position;
    }
}

demo_c01_result_t DemoC01_Start(demo_c01_context_t *ctx, UART_Type *uart)
{
    demo_c01_result_t ret = DemoC01_Init(ctx, uart);
    return ret == DEMO_C01_OK ? DemoC01_HomeBoth(ctx) : ret;
}

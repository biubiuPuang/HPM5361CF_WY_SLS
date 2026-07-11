#include "pedal_controller.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>

static void sleep_ms(int ms)
{
    if (ms > 0) {
        Sleep((DWORD)ms);
    }
}

static DWORD now_ms(void)
{
    return GetTickCount();
}

static int32_t abs_i32(int32_t value)
{
    return value < 0 ? -value : value;
}

static PedalResult set_last_error(PedalController *controller, PedalResult result)
{
    if (controller) {
        controller->last_error = result;
        if (result != PEDAL_OK) {
            controller->state = PEDAL_STATE_FAULT;
        }
    }
    return result;
}

static PedalResult check_open(PedalController *controller)
{
    if (!controller || !controller->is_initialized) {
        return PEDAL_ERR_INVALID_ARG;
    }
    if (!controller->is_open || !rs485_is_open(&controller->transport)) {
        return PEDAL_ERR_NOT_OPEN;
    }
    return PEDAL_OK;
}

static PedalResult check_soft_limit(PedalController *controller, int32_t target_cnt)
{
    if (!controller) {
        return PEDAL_ERR_INVALID_ARG;
    }
    if (target_cnt < controller->config.soft_limit_forward_cnt ||
        target_cnt > controller->config.soft_limit_backward_cnt) {
        return PEDAL_ERR_SOFT_LIMIT;
    }
    return PEDAL_OK;
}

static PedalResult ensure_position_profile(PedalController *controller)
{
    PedalResult result;

    if (controller->profile_valid &&
        controller->profile_accel_100ms == controller->config.move_accel_100ms &&
        controller->profile_decel_100ms == controller->config.move_decel_100ms &&
        controller->profile_max_rpm == controller->config.move_max_rpm) {
        return PEDAL_OK;
    }

    result = servo_axis_prepare_position_mode(
        &controller->brake,
        controller->config.move_accel_100ms,
        controller->config.move_decel_100ms,
        controller->config.move_max_rpm,
        1
    );
    if (result != PEDAL_OK) {
        return result;
    }

    sleep_ms(20);

    result = servo_axis_prepare_position_mode(
        &controller->accelerator,
        controller->config.move_accel_100ms,
        controller->config.move_decel_100ms,
        controller->config.move_max_rpm,
        1
    );
    if (result != PEDAL_OK) {
        return result;
    }

    controller->profile_valid = 1;
    controller->profile_accel_100ms = controller->config.move_accel_100ms;
    controller->profile_decel_100ms = controller->config.move_decel_100ms;
    controller->profile_max_rpm = controller->config.move_max_rpm;
    return PEDAL_OK;
}

static PedalResult wait_both_settled(PedalController *controller, int32_t target_cnt)
{
    DWORD start = now_ms();
    DWORD settle_start = 0;
    int settling = 0;
    AxisStatus brake_status;
    AxisStatus accelerator_status;

    while ((DWORD)(now_ms() - start) < (DWORD)controller->config.move_timeout_ms) {
        PedalResult result;
        int brake_ok;
        int accelerator_ok;

        result = servo_axis_read_fast_status(&controller->brake, &brake_status);
        if (result != PEDAL_OK) {
            return result;
        }

        sleep_ms(controller->config.inter_axis_poll_delay_ms);

        result = servo_axis_read_fast_status(&controller->accelerator, &accelerator_status);
        if (result != PEDAL_OK) {
            return result;
        }

        if (brake_status.has_fault || accelerator_status.has_fault) {
            return PEDAL_ERR_FAULT_STATUS;
        }

        brake_ok =
            abs_i32(brake_status.feedback_position_cnt - target_cnt) <= controller->config.move_tolerance_cnt &&
            abs(brake_status.motor_rpm) <= controller->config.move_rpm_threshold;
        accelerator_ok =
            abs_i32(accelerator_status.feedback_position_cnt - target_cnt) <= controller->config.move_tolerance_cnt &&
            abs(accelerator_status.motor_rpm) <= controller->config.move_rpm_threshold;

        if (brake_ok && accelerator_ok) {
            if (!settling) {
                settling = 1;
                settle_start = now_ms();
            } else if ((DWORD)(now_ms() - settle_start) >= (DWORD)controller->config.move_settle_hold_ms) {
                return PEDAL_OK;
            }
        } else {
            settling = 0;
        }

        sleep_ms(controller->config.move_poll_ms);
    }

    return PEDAL_ERR_MOVE_TIMEOUT;
}

PedalControllerConfig pedal_controller_default_config(const char *port_name)
{
    PedalControllerConfig config;
    memset(&config, 0, sizeof(config));

    config.port_name = port_name;
    config.baudrate = 38400;
    config.timeout_ms = 500;
    config.debug = 0;

    config.accelerator_pedal_id = 1;
    config.brake_pedal_id = 2;

    config.soft_limit_forward_cnt = -50000;
    config.soft_limit_backward_cnt = 50000;
    config.safe_retract_cnt = 0;

    config.max_dual_position_diff_cnt = 800;
    config.move_tolerance_cnt = 40;
    config.move_rpm_threshold = 40;
    config.move_settle_hold_ms = 120;
    config.move_timeout_ms = 12000;
    config.move_poll_ms = 50;

    config.move_max_rpm = 3000;
    config.move_accel_100ms = 1;
    config.move_decel_100ms = 2;

    config.move_post_command_delay_ms = 80;
    config.inter_axis_poll_delay_ms = 20;
    return config;
}

PedalResult pedal_controller_init(PedalController *controller, const PedalControllerConfig *config)
{
    if (!controller || !config || !config->port_name) {
        return PEDAL_ERR_INVALID_ARG;
    }

    memset(controller, 0, sizeof(*controller));
    controller->config = *config;
    controller->state = PEDAL_STATE_DISCONNECTED;
    controller->last_error = PEDAL_OK;
    controller->is_initialized = 1;
    rs485_init(&controller->transport);
    return PEDAL_OK;
}

PedalResult pedal_controller_open(PedalController *controller)
{
    RS485Config rs_cfg;
    PedalResult result;

    if (!controller || !controller->is_initialized) {
        return PEDAL_ERR_INVALID_ARG;
    }
    if (controller->is_open) {
        return PEDAL_OK;
    }

    memset(&rs_cfg, 0, sizeof(rs_cfg));
    rs_cfg.port_name = controller->config.port_name;
    rs_cfg.baudrate = controller->config.baudrate;
    rs_cfg.timeout_ms = controller->config.timeout_ms;
    rs_cfg.write_timeout_ms = controller->config.timeout_ms;
    rs_cfg.inter_frame_delay_ms = 4;
    rs_cfg.retry_count = 3;
    rs_cfg.debug = controller->config.debug;

    result = rs485_open(&controller->transport, &rs_cfg);
    if (result != PEDAL_OK) {
        return set_last_error(controller, result);
    }

    servo_axis_init(&controller->brake, controller->config.brake_pedal_id, &controller->transport);
    servo_axis_init(&controller->accelerator, controller->config.accelerator_pedal_id, &controller->transport);

    result = servo_axis_standard_init(&controller->brake, 1);
    if (result != PEDAL_OK) {
        rs485_close(&controller->transport);
        return set_last_error(controller, result);
    }

    result = servo_axis_standard_init(&controller->accelerator, 1);
    if (result != PEDAL_OK) {
        rs485_close(&controller->transport);
        return set_last_error(controller, result);
    }

    controller->is_open = 1;
    controller->state = PEDAL_STATE_READY;
    controller->last_error = PEDAL_OK;
    controller->profile_valid = 0;
    return PEDAL_OK;
}

void pedal_controller_close(PedalController *controller)
{
    if (!controller || !controller->is_initialized) {
        return;
    }
    if (controller->is_open) {
        (void)pedal_controller_stop_all(controller);
        sleep_ms(100);
    }
    rs485_close(&controller->transport);
    controller->is_open = 0;
    controller->state = PEDAL_STATE_DISCONNECTED;
    controller->profile_valid = 0;
}

PedalResult pedal_controller_move_all_absolute(PedalController *controller, int32_t target_cnt, int wait)
{
    PedalResult result;

    result = check_open(controller);
    if (result != PEDAL_OK) {
        return set_last_error(controller, result);
    }

    result = check_soft_limit(controller, target_cnt);
    if (result != PEDAL_OK) {
        return set_last_error(controller, result);
    }

    result = ensure_position_profile(controller);
    if (result != PEDAL_OK) {
        return set_last_error(controller, result);
    }

    result = servo_axis_set_target_position_cnt(&controller->brake, target_cnt);
    if (result != PEDAL_OK) {
        return set_last_error(controller, result);
    }

    sleep_ms(controller->config.move_post_command_delay_ms);

    result = servo_axis_set_target_position_cnt(&controller->accelerator, target_cnt);
    if (result != PEDAL_OK) {
        return set_last_error(controller, result);
    }

    controller->state = PEDAL_STATE_MOVING;

    if (!wait) {
        controller->last_error = PEDAL_OK;
        return PEDAL_OK;
    }

    result = wait_both_settled(controller, target_cnt);
    if (result != PEDAL_OK) {
        return set_last_error(controller, result);
    }

    controller->state = PEDAL_STATE_READY;
    controller->last_error = PEDAL_OK;
    return PEDAL_OK;
}

PedalResult pedal_controller_retract_to_safe(PedalController *controller, int wait)
{
    if (!controller) {
        return PEDAL_ERR_INVALID_ARG;
    }
    return pedal_controller_move_all_absolute(controller, controller->config.safe_retract_cnt, wait);
}

PedalResult pedal_controller_stop_all(PedalController *controller)
{
    PedalResult first_error = PEDAL_OK;
    PedalResult result;

    if (!controller || !controller->is_initialized) {
        return PEDAL_ERR_INVALID_ARG;
    }
    if (!controller->is_open) {
        return PEDAL_OK;
    }

    result = servo_axis_set_target_velocity_rpm(&controller->brake, 0);
    if (result != PEDAL_OK && first_error == PEDAL_OK) {
        first_error = result;
    }
    result = servo_axis_set_target_current_ma(&controller->brake, 0);
    if (result != PEDAL_OK && first_error == PEDAL_OK) {
        first_error = result;
    }
    result = servo_axis_buffered_stop(&controller->brake);
    if (result != PEDAL_OK && first_error == PEDAL_OK) {
        first_error = result;
    }

    result = servo_axis_set_target_velocity_rpm(&controller->accelerator, 0);
    if (result != PEDAL_OK && first_error == PEDAL_OK) {
        first_error = result;
    }
    result = servo_axis_set_target_current_ma(&controller->accelerator, 0);
    if (result != PEDAL_OK && first_error == PEDAL_OK) {
        first_error = result;
    }
    result = servo_axis_buffered_stop(&controller->accelerator);
    if (result != PEDAL_OK && first_error == PEDAL_OK) {
        first_error = result;
    }

    if (first_error == PEDAL_OK) {
        controller->state = PEDAL_STATE_READY;
    }
    controller->last_error = first_error;
    return first_error;
}

PedalResult pedal_controller_emergency_stop_all(PedalController *controller)
{
    PedalResult first_error = PEDAL_OK;
    PedalResult result;

    if (!controller || !controller->is_initialized) {
        return PEDAL_ERR_INVALID_ARG;
    }
    if (!controller->is_open) {
        return PEDAL_OK;
    }

    result = servo_axis_emergency_stop(&controller->brake);
    if (result != PEDAL_OK && first_error == PEDAL_OK) {
        first_error = result;
    }
    result = servo_axis_emergency_stop(&controller->accelerator);
    if (result != PEDAL_OK && first_error == PEDAL_OK) {
        first_error = result;
    }

    controller->last_error = first_error;
    return first_error;
}

PedalResult pedal_controller_status(PedalController *controller, PedalControllerStatus *out_status)
{
    PedalResult brake_result;
    PedalResult accelerator_result;

    if (!controller || !out_status) {
        return PEDAL_ERR_INVALID_ARG;
    }

    memset(out_status, 0, sizeof(*out_status));
    out_status->state = controller->state;
    out_status->last_error = controller->last_error;

    if (!controller->is_open) {
        out_status->state = PEDAL_STATE_DISCONNECTED;
        return PEDAL_OK;
    }

    brake_result = servo_axis_read_fast_status(&controller->brake, &out_status->brake);
    accelerator_result = servo_axis_read_fast_status(&controller->accelerator, &out_status->accelerator);

    out_status->brake_valid = brake_result == PEDAL_OK;
    out_status->accelerator_valid = accelerator_result == PEDAL_OK;

    if (out_status->brake_valid && out_status->accelerator_valid) {
        out_status->dual_position_diff_cnt = abs_i32(
            out_status->brake.feedback_position_cnt - out_status->accelerator.feedback_position_cnt
        );
        out_status->dual_sync_warning =
            out_status->dual_position_diff_cnt > controller->config.max_dual_position_diff_cnt;

        if (out_status->brake.has_fault || out_status->accelerator.has_fault) {
            out_status->state = PEDAL_STATE_FAULT;
        }
    }

    if (brake_result != PEDAL_OK) {
        out_status->last_error = brake_result;
        return brake_result;
    }
    if (accelerator_result != PEDAL_OK) {
        out_status->last_error = accelerator_result;
        return accelerator_result;
    }

    return PEDAL_OK;
}

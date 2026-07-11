#ifndef PEDAL_CONTROLLER_H
#define PEDAL_CONTROLLER_H

#include <stdint.h>

#include "servo_axis.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PedalControllerState {
    PEDAL_STATE_DISCONNECTED = 0,
    PEDAL_STATE_READY = 1,
    PEDAL_STATE_MOVING = 2,
    PEDAL_STATE_FAULT = 3
} PedalControllerState;

typedef struct PedalControllerConfig {
    const char *port_name;
    int baudrate;
    int timeout_ms;
    int debug;

    uint8_t accelerator_pedal_id;
    uint8_t brake_pedal_id;

    int32_t soft_limit_forward_cnt;
    int32_t soft_limit_backward_cnt;
    int32_t safe_retract_cnt;

    int max_dual_position_diff_cnt;
    int move_tolerance_cnt;
    int move_rpm_threshold;
    int move_settle_hold_ms;
    int move_timeout_ms;
    int move_poll_ms;

    int move_max_rpm;
    uint8_t move_accel_100ms;
    uint8_t move_decel_100ms;

    int move_post_command_delay_ms;
    int inter_axis_poll_delay_ms;
} PedalControllerConfig;

typedef struct PedalControllerStatus {
    PedalControllerState state;

    AxisStatus brake;
    AxisStatus accelerator;
    int brake_valid;
    int accelerator_valid;

    int32_t dual_position_diff_cnt;
    int dual_sync_warning;

    PedalResult last_error;
} PedalControllerStatus;

typedef struct PedalController {
    PedalControllerConfig config;
    RS485Transport transport;
    ServoAxis brake;
    ServoAxis accelerator;

    PedalControllerState state;
    PedalResult last_error;
    int is_initialized;
    int is_open;

    int profile_valid;
    uint8_t profile_accel_100ms;
    uint8_t profile_decel_100ms;
    int profile_max_rpm;
} PedalController;

PedalControllerConfig pedal_controller_default_config(const char *port_name);

PedalResult pedal_controller_init(PedalController *controller, const PedalControllerConfig *config);
PedalResult pedal_controller_open(PedalController *controller);
void pedal_controller_close(PedalController *controller);

PedalResult pedal_controller_move_all_absolute(PedalController *controller, int32_t target_cnt, int wait);
PedalResult pedal_controller_retract_to_safe(PedalController *controller, int wait);

PedalResult pedal_controller_stop_all(PedalController *controller);
PedalResult pedal_controller_emergency_stop_all(PedalController *controller);
PedalResult pedal_controller_status(PedalController *controller, PedalControllerStatus *out_status);

#ifdef __cplusplus
}
#endif

#endif /* PEDAL_CONTROLLER_H */

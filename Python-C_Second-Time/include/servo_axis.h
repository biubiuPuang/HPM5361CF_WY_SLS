#ifndef SERVO_AXIS_H
#define SERVO_AXIS_H

#include <stdint.h>

#include "pedal_error.h"
#include "rs485_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ServoAxis {
    uint8_t device_id;
    RS485Transport *transport;
} ServoAxis;

typedef struct AxisStatus {
    uint8_t device_id;
    uint16_t status_word;
    double output_current_a;
    int motor_rpm;
    int32_t feedback_position_cnt;
    int is_running;
    int has_fault;
} AxisStatus;

void servo_axis_init(ServoAxis *axis, uint8_t device_id, RS485Transport *transport);

PedalResult servo_axis_standard_init(ServoAxis *axis, int enable_comm_timeout);
PedalResult servo_axis_clear_fault(ServoAxis *axis);
PedalResult servo_axis_enable(ServoAxis *axis);
PedalResult servo_axis_disable(ServoAxis *axis);
PedalResult servo_axis_set_ctrl_state_pc(ServoAxis *axis);
PedalResult servo_axis_comm_timeout_enable(ServoAxis *axis);
PedalResult servo_axis_comm_timeout_disable(ServoAxis *axis);

PedalResult servo_axis_prepare_position_mode(
    ServoAxis *axis,
    uint8_t accel_100ms,
    uint8_t decel_100ms,
    int max_rpm,
    int absolute
);

PedalResult servo_axis_set_target_position_cnt(ServoAxis *axis, int32_t cnt);
PedalResult servo_axis_set_target_velocity_rpm(ServoAxis *axis, int rpm);
PedalResult servo_axis_set_target_current_ma(ServoAxis *axis, int ma);

PedalResult servo_axis_buffered_stop(ServoAxis *axis);
PedalResult servo_axis_emergency_stop(ServoAxis *axis);

PedalResult servo_axis_read_feedback_position_cnt(ServoAxis *axis, int32_t *out_cnt);
PedalResult servo_axis_read_fast_status(ServoAxis *axis, AxisStatus *out_status);

int servo_axis_status_word_has_fault(uint16_t status_word);
int servo_axis_status_word_is_running(uint16_t status_word);

#ifdef __cplusplus
}
#endif

#endif /* SERVO_AXIS_H */

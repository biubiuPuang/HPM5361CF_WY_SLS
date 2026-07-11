#include "servo_axis.h"

#include <stdint.h>

#include "ids830abs_registers.h"

static uint16_t pack_u16(uint8_t hi, uint8_t lo)
{
    return (uint16_t)(((uint16_t)hi << 8) | (uint16_t)lo);
}

static uint16_t pack_s16(int value)
{
    return (uint16_t)((int16_t)value);
}

static int16_t s16_from_u16(uint16_t value)
{
    return (int16_t)value;
}

static int32_t s32_from_words(uint16_t hi, uint16_t lo)
{
    uint32_t value = ((uint32_t)hi << 16) | (uint32_t)lo;
    return (int32_t)value;
}

static int round_div_i64(int64_t numerator, int64_t denominator)
{
    if (denominator == 0) {
        return 0;
    }
    if (numerator >= 0) {
        return (int)((numerator + denominator / 2) / denominator);
    }
    return (int)((numerator - denominator / 2) / denominator);
}

static uint16_t rpm_to_units(int rpm)
{
    int units = round_div_i64((int64_t)rpm * 8192, 3000);
    return (uint16_t)((int16_t)units);
}

static int units_to_rpm(int16_t units)
{
    return round_div_i64((int64_t)units * 3000, 8192);
}

void servo_axis_init(ServoAxis *axis, uint8_t device_id, RS485Transport *transport)
{
    if (!axis) {
        return;
    }
    axis->device_id = device_id;
    axis->transport = transport;
}

static PedalResult write_reg(ServoAxis *axis, uint16_t address, uint16_t value)
{
    if (!axis || !axis->transport) {
        return PEDAL_ERR_INVALID_ARG;
    }
    return rs485_write_single_register(axis->transport, axis->device_id, address, value);
}

PedalResult servo_axis_clear_fault(ServoAxis *axis)
{
    return write_reg(axis, IDS_REG_CLEAR_FAULT, 0x0000u);
}

PedalResult servo_axis_enable(ServoAxis *axis)
{
    return write_reg(axis, IDS_REG_ENABLE, 0x0001u);
}

PedalResult servo_axis_disable(ServoAxis *axis)
{
    return write_reg(axis, IDS_REG_ENABLE, 0x0000u);
}

PedalResult servo_axis_set_ctrl_state_pc(ServoAxis *axis)
{
    return write_reg(axis, IDS_REG_CTRL_STATE, IDS_CTRL_STATE_PC);
}

PedalResult servo_axis_comm_timeout_enable(ServoAxis *axis)
{
    return write_reg(axis, IDS_REG_COMM_TIMEOUT, IDS_COMM_TIMEOUT_ENABLE);
}

PedalResult servo_axis_comm_timeout_disable(ServoAxis *axis)
{
    return write_reg(axis, IDS_REG_COMM_TIMEOUT, IDS_COMM_TIMEOUT_DISABLE);
}

PedalResult servo_axis_standard_init(ServoAxis *axis, int enable_comm_timeout)
{
    PedalResult result;

    result = servo_axis_clear_fault(axis);
    if (result != PEDAL_OK) {
        return result;
    }

    result = servo_axis_set_ctrl_state_pc(axis);
    if (result != PEDAL_OK) {
        return result;
    }

    if (enable_comm_timeout) {
        return servo_axis_comm_timeout_enable(axis);
    }
    return servo_axis_comm_timeout_disable(axis);
}

PedalResult servo_axis_prepare_position_mode(
    ServoAxis *axis,
    uint8_t accel_100ms,
    uint8_t decel_100ms,
    int max_rpm,
    int absolute
)
{
    PedalResult result;

    result = write_reg(axis, IDS_REG_MODE_SELECT, IDS_MODE_POS_PC);
    if (result != PEDAL_OK) {
        return result;
    }

    result = write_reg(axis, IDS_REG_POS_ACCEL, pack_u16(accel_100ms, decel_100ms));
    if (result != PEDAL_OK) {
        return result;
    }

    result = write_reg(axis, IDS_REG_MAX_VEL_LIMIT, rpm_to_units(max_rpm));
    if (result != PEDAL_OK) {
        return result;
    }

    result = write_reg(axis, IDS_REG_ABS_REL, absolute ? 0x0000u : 0x0001u);
    if (result != PEDAL_OK) {
        return result;
    }

    return servo_axis_enable(axis);
}

PedalResult servo_axis_set_target_position_cnt(ServoAxis *axis, int32_t cnt)
{
    if (!axis || !axis->transport) {
        return PEDAL_ERR_INVALID_ARG;
    }
    return rs485_write_position_32(axis->transport, axis->device_id, IDS_REG_POS_HIGH, cnt);
}

PedalResult servo_axis_set_target_velocity_rpm(ServoAxis *axis, int rpm)
{
    return write_reg(axis, IDS_REG_VEL_TARGET, pack_s16((int16_t)rpm_to_units(rpm)));
}

PedalResult servo_axis_set_target_current_ma(ServoAxis *axis, int ma)
{
    return write_reg(axis, IDS_REG_TORQUE_TARGET, pack_s16(ma));
}

PedalResult servo_axis_buffered_stop(ServoAxis *axis)
{
    return write_reg(axis, IDS_REG_BUF_STOP, 0x0000u);
}

PedalResult servo_axis_emergency_stop(ServoAxis *axis)
{
    return write_reg(axis, IDS_REG_EMG_STOP, 0x0000u);
}

PedalResult servo_axis_read_feedback_position_cnt(ServoAxis *axis, int32_t *out_cnt)
{
    uint16_t values[2];
    PedalResult result;

    if (!axis || !out_cnt) {
        return PEDAL_ERR_INVALID_ARG;
    }

    result = rs485_read_holding_registers(axis->transport, axis->device_id, IDS_M_R_FEEDBACK_POS, 2, values);
    if (result != PEDAL_OK) {
        return result;
    }

    *out_cnt = s32_from_words(values[0], values[1]);
    return PEDAL_OK;
}

PedalResult servo_axis_read_fast_status(ServoAxis *axis, AxisStatus *out_status)
{
    uint16_t values[5];
    int16_t rpm_units;
    PedalResult result;

    if (!axis || !out_status) {
        return PEDAL_ERR_INVALID_ARG;
    }

    result = rs485_read_holding_registers(axis->transport, axis->device_id, IDS_M_R_FAST_MONITOR, 5, values);
    if (result != PEDAL_OK) {
        return result;
    }

    rpm_units = s16_from_u16(values[2]);
    out_status->device_id = axis->device_id;
    out_status->status_word = values[0];
    out_status->output_current_a = values[1] / 100.0;
    out_status->motor_rpm = units_to_rpm(rpm_units);
    out_status->feedback_position_cnt = s32_from_words(values[3], values[4]);
    out_status->is_running = servo_axis_status_word_is_running(values[0]);
    out_status->has_fault = servo_axis_status_word_has_fault(values[0]);
    return PEDAL_OK;
}

int servo_axis_status_word_is_running(uint16_t status_word)
{
    return (status_word & (uint16_t)(1u << IDS_STATUS_RUNNING_BIT)) != 0;
}

int servo_axis_status_word_has_fault(uint16_t status_word)
{
    const uint16_t fault_mask =
        (uint16_t)(1u << IDS_STATUS_OVER_CURRENT) |
        (uint16_t)(1u << IDS_STATUS_OVER_VOLTAGE) |
        (uint16_t)(1u << IDS_STATUS_ENCODER_ERROR) |
        (uint16_t)(1u << IDS_STATUS_POSITION_ERROR) |
        (uint16_t)(1u << IDS_STATUS_UNDER_VOLTAGE) |
        (uint16_t)(1u << IDS_STATUS_OVERLOAD);

    return (status_word & fault_mask) != 0;
}

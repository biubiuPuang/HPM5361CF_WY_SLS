#include "pedal_error.h"

const char *pedal_result_to_string(PedalResult result)
{
    switch (result) {
    case PEDAL_OK:
        return "OK";
    case PEDAL_ERR_INVALID_ARG:
        return "invalid argument";
    case PEDAL_ERR_NOT_OPEN:
        return "RS485 port/controller is not open";
    case PEDAL_ERR_SERIAL_OPEN:
        return "failed to open serial port";
    case PEDAL_ERR_SERIAL_IO:
        return "serial I/O error";
    case PEDAL_ERR_TIMEOUT:
        return "serial/Modbus timeout";
    case PEDAL_ERR_MODBUS_CRC:
        return "Modbus CRC mismatch";
    case PEDAL_ERR_MODBUS_EXCEPTION:
        return "Modbus exception response";
    case PEDAL_ERR_RESPONSE_MISMATCH:
        return "Modbus response mismatch";
    case PEDAL_ERR_SOFT_LIMIT:
        return "target is outside software limits";
    case PEDAL_ERR_MOVE_TIMEOUT:
        return "motion did not settle before timeout";
    case PEDAL_ERR_DUAL_POSITION_DIFF:
        return "dual pedal position difference is too large";
    case PEDAL_ERR_FAULT_STATUS:
        return "servo status word reports a fault";
    default:
        return "unknown error";
    }
}

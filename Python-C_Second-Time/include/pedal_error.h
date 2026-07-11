#ifndef PEDAL_ERROR_H
#define PEDAL_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PedalResult {
    PEDAL_OK = 0,
    PEDAL_ERR_INVALID_ARG = -1,
    PEDAL_ERR_NOT_OPEN = -2,
    PEDAL_ERR_SERIAL_OPEN = -3,
    PEDAL_ERR_SERIAL_IO = -4,
    PEDAL_ERR_TIMEOUT = -5,
    PEDAL_ERR_MODBUS_CRC = -6,
    PEDAL_ERR_MODBUS_EXCEPTION = -7,
    PEDAL_ERR_RESPONSE_MISMATCH = -8,
    PEDAL_ERR_SOFT_LIMIT = -9,
    PEDAL_ERR_MOVE_TIMEOUT = -10,
    PEDAL_ERR_DUAL_POSITION_DIFF = -11,
    PEDAL_ERR_FAULT_STATUS = -12
} PedalResult;

const char *pedal_result_to_string(PedalResult result);

#ifdef __cplusplus
}
#endif

#endif /* PEDAL_ERROR_H */

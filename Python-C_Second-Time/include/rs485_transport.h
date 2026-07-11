#ifndef RS485_TRANSPORT_H
#define RS485_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "pedal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RS485Config {
    const char *port_name;
    int baudrate;
    int timeout_ms;
    int write_timeout_ms;
    int inter_frame_delay_ms;
    int retry_count;
    int debug;
} RS485Config;

typedef struct RS485Transport {
    void *handle;
    void *lock;
    RS485Config config;
    int is_open;
} RS485Transport;

void rs485_init(RS485Transport *transport);
PedalResult rs485_open(RS485Transport *transport, const RS485Config *config);
void rs485_close(RS485Transport *transport);
int rs485_is_open(const RS485Transport *transport);

uint16_t modbus_crc16(const uint8_t *data, size_t len);

PedalResult rs485_read_holding_registers(
    RS485Transport *transport,
    uint8_t slave_id,
    uint16_t address,
    uint16_t count,
    uint16_t *out_values
);

PedalResult rs485_write_single_register(
    RS485Transport *transport,
    uint8_t slave_id,
    uint16_t address,
    uint16_t value
);

PedalResult rs485_write_position_32(
    RS485Transport *transport,
    uint8_t slave_id,
    uint16_t start_address,
    int32_t value
);

#ifdef __cplusplus
}
#endif

#endif /* RS485_TRANSPORT_H */

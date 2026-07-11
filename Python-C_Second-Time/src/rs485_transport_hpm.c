#include "rs485_transport.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "drv_rs485.h"
#include "drv_timer_extra.h"

#define MODBUS_MAX_FRAME 260u

static void sleep_ms(int ms)
{
    if (ms > 0) {
        board_delay_ms((uint32_t)ms);
    }
}

static void debug_dump(const char *prefix, const uint8_t *data, size_t len)
{
    size_t i;

    printf("%s", prefix);
    for (i = 0; i < len; ++i) {
        printf("%02X", data[i]);
        if (i + 1u < len) {
            printf(" ");
        }
    }
    printf("\r\n");
}

static PedalResult status_to_pedal_result(hpm_stat_t status)
{
    if (status == status_success) {
        return PEDAL_OK;
    }
    if (status == status_timeout) {
        return PEDAL_ERR_TIMEOUT;
    }
    if (status == status_invalid_argument) {
        return PEDAL_ERR_INVALID_ARG;
    }
    return PEDAL_ERR_SERIAL_IO;
}

static PedalResult parse_channel_name(const char *port_name, rs485_channel_t *out_ch)
{
    if (!port_name || !out_ch) {
        return PEDAL_ERR_INVALID_ARG;
    }

    if (strcmp(port_name, "RS485_CH1") == 0) {
        *out_ch = RS485_CH1;
        return PEDAL_OK;
    }
    if (strcmp(port_name, "RS485_CH2") == 0) {
        *out_ch = RS485_CH2;
        return PEDAL_OK;
    }

    return PEDAL_ERR_SERIAL_OPEN;
}

static rs485_channel_t transport_channel(const RS485Transport *transport)
{
    return (rs485_channel_t)((uintptr_t)transport->handle - 1u);
}

static void transport_set_channel(RS485Transport *transport, rs485_channel_t ch)
{
    transport->handle = (void *)(uintptr_t)((uint32_t)ch + 1u);
}

static PedalResult read_exact(RS485Transport *transport, uint8_t *buf, size_t len)
{
    rs485_channel_t ch;
    size_t got = 0;
    uint32_t start_ms;
    uint32_t timeout_ms;

    if (!transport || !transport->is_open || !transport->handle || !buf) {
        return PEDAL_ERR_NOT_OPEN;
    }
    if (len == 0u) {
        return PEDAL_OK;
    }

    ch = transport_channel(transport);
    timeout_ms = (uint32_t)(transport->config.timeout_ms > 0 ? transport->config.timeout_ms : 500);
    start_ms = system_get_time_ms();

    while (got < len) {
        uint32_t available = rs485_rx_available(ch);
        if (available > 0u) {
            uint32_t need = (uint32_t)(len - got);
            uint32_t to_read = available < need ? available : need;
            uint32_t nread = rs485_rx_read(ch, buf + got, to_read);
            got += nread;
            continue;
        }

        if ((uint32_t)(system_get_time_ms() - start_ms) >= timeout_ms) {
            return PEDAL_ERR_TIMEOUT;
        }
        board_delay_ms(1);
    }

    return PEDAL_OK;
}

static PedalResult write_all(RS485Transport *transport, const uint8_t *buf, size_t len)
{
    hpm_stat_t status;

    if (!transport || !transport->is_open || !transport->handle || !buf) {
        return PEDAL_ERR_NOT_OPEN;
    }
    if (len == 0u || len > RS485_DMA_BUFFER_SIZE) {
        return PEDAL_ERR_INVALID_ARG;
    }

    status = rs485_send_data(transport_channel(transport), buf, (uint32_t)len);
    if (status == status_success) {
        board_delay_us(1000);
    }
    return status_to_pedal_result(status);
}

static void append_crc(uint8_t *frame, size_t len_without_crc)
{
    uint16_t crc = modbus_crc16(frame, len_without_crc);
    frame[len_without_crc] = (uint8_t)(crc & 0xFFu);
    frame[len_without_crc + 1u] = (uint8_t)((crc >> 8) & 0xFFu);
}

static PedalResult check_crc(const uint8_t *frame, size_t len)
{
    uint16_t got;
    uint16_t expected;

    if (!frame || len < 4u) {
        return PEDAL_ERR_RESPONSE_MISMATCH;
    }

    got = (uint16_t)(frame[len - 2u] | ((uint16_t)frame[len - 1u] << 8));
    expected = modbus_crc16(frame, len - 2u);
    if (got != expected) {
        return PEDAL_ERR_MODBUS_CRC;
    }
    return PEDAL_OK;
}

static PedalResult modbus_request_unlocked(
    RS485Transport *transport,
    uint8_t slave_id,
    uint8_t function,
    const uint8_t *payload,
    size_t payload_len,
    uint8_t *response,
    size_t response_len
)
{
    uint8_t request[MODBUS_MAX_FRAME];
    size_t request_len;
    int attempt;
    int retries;
    PedalResult last_result = PEDAL_ERR_TIMEOUT;
    rs485_channel_t ch;

    if (!transport || !response || !payload || payload_len + 4u > sizeof(request)) {
        return PEDAL_ERR_INVALID_ARG;
    }
    if (!transport->is_open || !transport->handle) {
        return PEDAL_ERR_NOT_OPEN;
    }
    if (slave_id < 1u || slave_id > 247u) {
        return PEDAL_ERR_INVALID_ARG;
    }

    ch = transport_channel(transport);
    request[0] = slave_id;
    request[1] = function;
    memcpy(request + 2, payload, payload_len);
    request_len = payload_len + 2u;
    append_crc(request, request_len);
    request_len += 2u;

    retries = transport->config.retry_count > 0 ? transport->config.retry_count : 3;
    for (attempt = 0; attempt < retries; ++attempt) {
        PedalResult result;

        rs485_rx_flush(ch);
        if (transport->config.debug) {
            debug_dump("TX: ", request, request_len);
        }

        result = write_all(transport, request, request_len);
        if (result != PEDAL_OK) {
            return result;
        }

        result = read_exact(transport, response, 2u);
        if (result == PEDAL_OK) {
            size_t actual_response_len = response_len;

            if (response[1] == (uint8_t)(function | 0x80u)) {
                actual_response_len = 5u;
                result = read_exact(transport, response + 2u, actual_response_len - 2u);
            } else if (response_len >= 2u) {
                result = read_exact(transport, response + 2u, response_len - 2u);
            } else {
                result = PEDAL_ERR_RESPONSE_MISMATCH;
            }

            if (result != PEDAL_OK) {
                last_result = result;
            } else {
                if (transport->config.debug) {
                    debug_dump("RX: ", response, actual_response_len);
                }

                result = check_crc(response, actual_response_len);
                if (result != PEDAL_OK) {
                    return result;
                }
                if (response[0] != slave_id) {
                    return PEDAL_ERR_RESPONSE_MISMATCH;
                }
                if (response[1] == (uint8_t)(function | 0x80u)) {
                    return PEDAL_ERR_MODBUS_EXCEPTION;
                }
                if (response[1] != function) {
                    return PEDAL_ERR_RESPONSE_MISMATCH;
                }
                sleep_ms(transport->config.inter_frame_delay_ms);
                return PEDAL_OK;
            }
        }

        last_result = result;
        if (result != PEDAL_ERR_TIMEOUT) {
            return result;
        }
        sleep_ms(60);
    }

    return last_result;
}

static PedalResult modbus_request(
    RS485Transport *transport,
    uint8_t slave_id,
    uint8_t function,
    const uint8_t *payload,
    size_t payload_len,
    uint8_t *response,
    size_t response_len
)
{
    return modbus_request_unlocked(
        transport,
        slave_id,
        function,
        payload,
        payload_len,
        response,
        response_len
    );
}

void rs485_init(RS485Transport *transport)
{
    if (!transport) {
        return;
    }
    memset(transport, 0, sizeof(*transport));
}

PedalResult rs485_open(RS485Transport *transport, const RS485Config *config)
{
    RS485Config cfg;
    rs485_channel_t ch;
    PedalResult result;
    hpm_stat_t status;

    if (!transport || !config || !config->port_name) {
        return PEDAL_ERR_INVALID_ARG;
    }
    if (transport->is_open) {
        return PEDAL_OK;
    }

    result = parse_channel_name(config->port_name, &ch);
    if (result != PEDAL_OK) {
        return result;
    }

    cfg = *config;
    if (cfg.baudrate <= 0) {
        cfg.baudrate = 38400;
    }
    if (cfg.timeout_ms <= 0) {
        cfg.timeout_ms = 500;
    }
    if (cfg.write_timeout_ms <= 0) {
        cfg.write_timeout_ms = cfg.timeout_ms;
    }
    if (cfg.inter_frame_delay_ms <= 0) {
        cfg.inter_frame_delay_ms = 4;
    }
    if (cfg.retry_count <= 0) {
        cfg.retry_count = 3;
    }

    status = rs485_init_channel(ch, (uint32_t)cfg.baudrate);
    if (status != status_success) {
        return status_to_pedal_result(status);
    }

    rs485_rx_flush(ch);
    transport_set_channel(transport, ch);
    transport->config = cfg;
    transport->is_open = 1;
    return PEDAL_OK;
}

void rs485_close(RS485Transport *transport)
{
    if (!transport) {
        return;
    }
    if (transport->is_open && transport->handle) {
        rs485_rx_flush(transport_channel(transport));
    }
    transport->handle = NULL;
    transport->lock = NULL;
    transport->is_open = 0;
}

int rs485_is_open(const RS485Transport *transport)
{
    return transport && transport->is_open;
}

uint16_t modbus_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    size_t i;

    if (!data && len > 0u) {
        return 0u;
    }

    for (i = 0; i < len; ++i) {
        int bit;
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            if (crc & 0x0001u) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001u);
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

PedalResult rs485_read_holding_registers(
    RS485Transport *transport,
    uint8_t slave_id,
    uint16_t address,
    uint16_t count,
    uint16_t *out_values
)
{
    uint8_t payload[4];
    uint8_t response[MODBUS_MAX_FRAME];
    size_t response_len;
    PedalResult result;
    uint16_t i;

    if (!transport || !out_values || count == 0u || count > 125u) {
        return PEDAL_ERR_INVALID_ARG;
    }

    response_len = 5u + (size_t)count * 2u;
    if (response_len > sizeof(response)) {
        return PEDAL_ERR_INVALID_ARG;
    }

    payload[0] = (uint8_t)((address >> 8) & 0xFFu);
    payload[1] = (uint8_t)(address & 0xFFu);
    payload[2] = (uint8_t)((count >> 8) & 0xFFu);
    payload[3] = (uint8_t)(count & 0xFFu);

    result = modbus_request(transport, slave_id, 0x03u, payload, sizeof(payload), response, response_len);
    if (result != PEDAL_OK) {
        return result;
    }

    if (response[2] != (uint8_t)(count * 2u)) {
        return PEDAL_ERR_RESPONSE_MISMATCH;
    }

    for (i = 0; i < count; ++i) {
        size_t off = 3u + (size_t)i * 2u;
        out_values[i] = (uint16_t)(((uint16_t)response[off] << 8) | response[off + 1u]);
    }
    return PEDAL_OK;
}

PedalResult rs485_write_single_register(
    RS485Transport *transport,
    uint8_t slave_id,
    uint16_t address,
    uint16_t value
)
{
    uint8_t payload[4];
    uint8_t response[8];
    PedalResult result;

    if (!transport) {
        return PEDAL_ERR_INVALID_ARG;
    }

    payload[0] = (uint8_t)((address >> 8) & 0xFFu);
    payload[1] = (uint8_t)(address & 0xFFu);
    payload[2] = (uint8_t)((value >> 8) & 0xFFu);
    payload[3] = (uint8_t)(value & 0xFFu);

    result = modbus_request(transport, slave_id, 0x06u, payload, sizeof(payload), response, sizeof(response));
    if (result != PEDAL_OK) {
        return result;
    }

    if (memcmp(response + 2, payload, sizeof(payload)) != 0) {
        return PEDAL_ERR_RESPONSE_MISMATCH;
    }
    return PEDAL_OK;
}

PedalResult rs485_write_position_32(
    RS485Transport *transport,
    uint8_t slave_id,
    uint16_t start_address,
    int32_t value
)
{
    uint8_t payload[9];
    uint8_t response[8];
    uint32_t raw = (uint32_t)value;
    PedalResult result;

    if (!transport) {
        return PEDAL_ERR_INVALID_ARG;
    }

    payload[0] = (uint8_t)((start_address >> 8) & 0xFFu);
    payload[1] = (uint8_t)(start_address & 0xFFu);
    payload[2] = 0x00u;
    payload[3] = 0x02u;
    payload[4] = 0x04u;
    payload[5] = (uint8_t)((raw >> 24) & 0xFFu);
    payload[6] = (uint8_t)((raw >> 16) & 0xFFu);
    payload[7] = (uint8_t)((raw >> 8) & 0xFFu);
    payload[8] = (uint8_t)(raw & 0xFFu);

    result = modbus_request(transport, slave_id, 0x10u, payload, sizeof(payload), response, sizeof(response));
    if (result != PEDAL_OK) {
        return result;
    }

    if (response[2] != payload[0] || response[3] != payload[1] ||
        response[4] != 0x00u || response[5] != 0x02u) {
        return PEDAL_ERR_RESPONSE_MISMATCH;
    }
    return PEDAL_OK;
}

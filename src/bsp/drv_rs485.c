#include "drv_rs485.h"
#include <string.h>
#include "ring_buffer.h"
#include "drv_timer_extra.h"

/* ============================================================
 *  全局变量
 * ============================================================ */
ATTR_PLACE_AT_NONCACHEABLE static uint8_t rs485_dma_rx_buffer[RS485_CHANNEL_NUM][RS485_DMA_BUFFER_SIZE];
extern volatile uint32_t monitor_timeout;
extern volatile bool busTimeoutFlag;

/* 每路 485 一个独立环形缓冲 + 兜底存储 */
#define RS485_RB_SIZE 512U
static uint8_t rs485_rb_buf[RS485_CHANNEL_NUM][RS485_RB_SIZE];
static ring_buffer_t rs485_rb[RS485_CHANNEL_NUM];

static bool rs485_channel_is_valid(rs485_channel_t ch)
{
    return ch < RS485_CHANNEL_NUM;
}

static rs485_ctx_t rs485_ctx[RS485_CHANNEL_NUM] = {
    [RS485_CH1] = {
        .uart = RS485_1_UART,
        .clk_name = RS485_1_UART_CLK_NAME,
        .irq = RS485_1_UART_IRQ,
        .baudrate = RS485_1_UART_BAUDRATE,
        .tx_dma_chn = RS485_1_UART_TX_DMA_CHN,
        .rx_dma_chn = RS485_1_UART_RX_DMA_CHN,
        .tx_dma_req = RS485_1_UART_TX_DMA_REQ,
        .rx_dma_req = RS485_1_UART_RX_DMA_REQ,
        .de_gpio = RS485_1_DE_GPIO,
        .de_port = RS485_1_DE_PORT,
        .de_pin = RS485_1_DE_PIN,
        .tx_done = false,
        .rx_idle = false,
        .rx_len = 0,
    },
    [RS485_CH2] = {
        .uart = RS485_2_UART,
        .clk_name = RS485_2_UART_CLK_NAME,
        .irq = RS485_2_UART_IRQ,
        .baudrate = RS485_2_UART_BAUDRATE,
        .tx_dma_chn = RS485_2_UART_TX_DMA_CHN,
        .rx_dma_chn = RS485_2_UART_RX_DMA_CHN,
        .tx_dma_req = RS485_2_UART_TX_DMA_REQ,
        .rx_dma_req = RS485_2_UART_RX_DMA_REQ,
        .de_gpio = RS485_2_DE_GPIO,
        .de_port = RS485_2_DE_PORT,
        .de_pin = RS485_2_DE_PIN,
        .tx_done = false,
        .rx_idle = false,
        .rx_len = 0,
    },
};

/* ============================================================
 *  DMA回调
 * ============================================================ */
static void rs485_dma_callback(uint8_t channel)
{
    if (channel == RS485_1_UART_RX_DMA_CHN)
    {
        rs485_ctx[RS485_CH1].rx_idle = true;
    }
    else if (channel == RS485_1_UART_TX_DMA_CHN)
    {
        rs485_ctx[RS485_CH1].tx_done = true;
    }
    else if (channel == RS485_2_UART_RX_DMA_CHN)
    {
        rs485_ctx[RS485_CH2].rx_idle = true;
    }
    else if (channel == RS485_2_UART_TX_DMA_CHN)
    {
        rs485_ctx[RS485_CH2].tx_done = true;
    }
}

/* ============================================================
 *  UART中断
 * ============================================================ */
void rs485_1_uart_isr(void)
{
    if (uart_is_rxline_idle(rs485_ctx[RS485_CH1].uart))
    {
        rs485_ctx[RS485_CH1].rx_idle = true;
        uart_clear_rxline_idle_flag(rs485_ctx[RS485_CH1].uart);
        uart_flush(rs485_ctx[RS485_CH1].uart);
    }
}
SDK_DECLARE_EXT_ISR_M(RS485_1_UART_IRQ, rs485_1_uart_isr)

void rs485_2_uart_isr(void)
{
    if (uart_is_rxline_idle(rs485_ctx[RS485_CH2].uart))
    {
        rs485_ctx[RS485_CH2].rx_idle = true;
        uart_clear_rxline_idle_flag(rs485_ctx[RS485_CH2].uart);
        uart_flush(rs485_ctx[RS485_CH2].uart);
    }
}
SDK_DECLARE_EXT_ISR_M(RS485_2_UART_IRQ, rs485_2_uart_isr)

/* ============================================================
 *  模式控制
 * ============================================================ */
void rs485_set_mode(rs485_channel_t ch, uint8_t mode)
{
    if (!rs485_channel_is_valid(ch))
    {
        return;
    }
    gpio_write_pin(rs485_ctx[ch].de_gpio, rs485_ctx[ch].de_port, rs485_ctx[ch].de_pin, mode);
}

/* ============================================================
 *  DMA配置
 * ============================================================ */
static hpm_stat_t rs485_configure_dma_rx(rs485_channel_t ch)
{
    hpm_stat_t status;

    if (!rs485_channel_is_valid(ch))
    {
        return status_invalid_argument;
    }
    dma_handshake_config_t config;

    dma_disable_channel(BOARD_APP_HDMA, rs485_ctx[ch].rx_dma_chn);
    uart_clear_rxline_idle_flag(rs485_ctx[ch].uart);

    dma_default_handshake_config(BOARD_APP_HDMA, &config);
    config.ch_index = rs485_ctx[ch].rx_dma_chn;
    config.dst = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)rs485_dma_rx_buffer[ch]);
    config.dst_fixed = false;
    config.src = (uint32_t)&rs485_ctx[ch].uart->RBR;
    config.src_fixed = true;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = RS485_DMA_BUFFER_SIZE;

    status = dma_setup_handshake(BOARD_APP_HDMA, &config, true);
    rs485_ctx[ch].rx_idle = false;

    return status;
}

// 重新配置DMA接收
hpm_stat_t rs485_reconfig_dma_rx(rs485_channel_t ch)
{
    if (!rs485_channel_is_valid(ch))
    {
        return status_invalid_argument;
    }
    dma_disable_channel(BOARD_APP_HDMA, rs485_ctx[ch].rx_dma_chn);
    memset(rs485_dma_rx_buffer[ch], 0, RS485_DMA_BUFFER_SIZE);
    return rs485_configure_dma_rx(ch);
}

static hpm_stat_t rs485_uart_tx_trigger_dma(rs485_channel_t ch, uint32_t src, uint32_t size)
{
    dma_handshake_config_t config = {0};

    dma_default_handshake_config(BOARD_APP_HDMA, &config);
    config.ch_index = rs485_ctx[ch].tx_dma_chn;
    config.dst = (uint32_t)&rs485_ctx[ch].uart->THR;
    config.dst_fixed = true;
    config.src = src;
    config.src_fixed = false;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;

    return dma_setup_handshake(BOARD_APP_HDMA, &config, true);
}

/* ============================================================
 *  初始化
 * ============================================================ */
hpm_stat_t rs485_init_channel(rs485_channel_t ch, uint32_t baudrate)
{
    hpm_stat_t status;
    uart_config_t config = {0};

    if (!rs485_channel_is_valid(ch))
    {
        return status_invalid_argument;
    }
    if (baudrate > 0)
    {
        rs485_ctx[ch].baudrate = baudrate;
    }

    /* DE引脚: 设为GPIO输出, 默认拉低(接收模式) */
    gpio_set_pin_output(rs485_ctx[ch].de_gpio, rs485_ctx[ch].de_port, rs485_ctx[ch].de_pin);
    rs485_set_mode(ch, RS485_MODE_RX);

    /* UART引脚初始化(由board.c的init_uart_pins完成) */
    board_init_uart(rs485_ctx[ch].uart);

    /* UART配置 */
    uart_default_config(rs485_ctx[ch].uart, &config);
    config.fifo_enable = true;
    config.dma_enable = true;
    config.src_freq_in_hz = clock_get_frequency(rs485_ctx[ch].clk_name);
    config.tx_fifo_level = uart_tx_fifo_trg_not_full;
    config.rx_fifo_level = uart_rx_fifo_trg_not_empty;
    config.baudrate = rs485_ctx[ch].baudrate;

    config.rxidle_config.detect_enable = true;
    config.rxidle_config.detect_irq_enable = true;
    config.rxidle_config.idle_cond = uart_rxline_idle_cond_rxline_logic_one;
    config.rxidle_config.threshold = RS485_IDLE_THRESHOLD;

    status = uart_init(rs485_ctx[ch].uart, &config);
    if (status != status_success)
    {
        return status;
    }

    /* DMA通道: dmamux用 ch_index 对应通道号 */
    dmamux_config(BOARD_APP_DMAMUX, rs485_ctx[ch].rx_dma_chn, rs485_ctx[ch].rx_dma_req, true);
    dmamux_config(BOARD_APP_DMAMUX, rs485_ctx[ch].tx_dma_chn, rs485_ctx[ch].tx_dma_req, true);

    dma_register_callback(rs485_ctx[ch].rx_dma_chn, rs485_dma_callback);
    dma_register_callback(rs485_ctx[ch].tx_dma_chn, rs485_dma_callback);

    status = rs485_configure_dma_rx(ch);
    if (status != status_success)
    {
        return status;
    }

    intc_m_enable_irq_with_priority(rs485_ctx[ch].irq, 2);
    uart_enable_irq(rs485_ctx[ch].uart, uart_intr_rx_line_idle);

    rs485_ctx[ch].tx_done = false;
    rs485_ctx[ch].rx_idle = false;
    rs485_ctx[ch].rx_len = 0;

    // 初始化当前通道接收缓冲区
    ring_buffer_init(&rs485_rb[ch], rs485_rb_buf[ch], RS485_RB_SIZE);

    return status_success;
}

hpm_stat_t old_rs485_init(rs485_channel_t ch)
{
    if (!rs485_channel_is_valid(ch))
    {
        return status_invalid_argument;
    }
    return rs485_init_channel(ch, rs485_ctx[ch].baudrate);
}

/* ============================================================
 *  发送
 * ============================================================ */
hpm_stat_t rs485_send_data(rs485_channel_t ch, const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0 || len > RS485_DMA_BUFFER_SIZE)
    {
        return status_invalid_argument;
    }
    if (!rs485_channel_is_valid(ch))
    {
        return status_invalid_argument;
    }

    /* 切到发送模式 */
    rs485_set_mode(ch, RS485_MODE_TX);
    rs485_ctx[ch].tx_done = false;

    hpm_stat_t status = rs485_uart_tx_trigger_dma(
        ch,
        core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)data),
        len);

    if (status != status_success)
    {
        rs485_set_mode(ch, RS485_MODE_RX);
        return status;
    }

    /* 等待DMA发送完成 */
    uint32_t timeout = RS485_TX_TIMEOUT;
    while (!rs485_ctx[ch].tx_done && timeout > 0)
    {
        board_delay_us(100);
        timeout--;
    }

    /* 切回接收模式 */
    rs485_set_mode(ch, RS485_MODE_RX);

    return (timeout == 0) ? status_timeout : status_success;
}

/* ============================================================
 *  接收
 * ============================================================ */
uint32_t rs485_get_dma_received_bytes(rs485_channel_t ch)
{
    if (!rs485_channel_is_valid(ch))
    {
        return 0;
    }
    uint32_t remaining = dma_get_remaining_transfer_size(BOARD_APP_HDMA, rs485_ctx[ch].rx_dma_chn);
    return (remaining < RS485_DMA_BUFFER_SIZE) ? (RS485_DMA_BUFFER_SIZE - remaining) : 0;
}

uint32_t rs485_check_and_handle_rx(rs485_channel_t ch)
{
    if (!rs485_channel_is_valid(ch))
    {
        return 0;
    }
    if (!rs485_ctx[ch].rx_idle)
        return 0;

    uint32_t writelen = 0;
    uint32_t received_bytes = rs485_get_dma_received_bytes(ch);

    if (received_bytes > 0)
    {
        uint32_t copy_len = (received_bytes < RS485_DMA_BUFFER_SIZE) ? received_bytes : RS485_DMA_BUFFER_SIZE;
        writelen = ring_buffer_write(&rs485_rb[ch], rs485_dma_rx_buffer[ch], copy_len);
        rs485_ctx[ch].rx_len = copy_len;
    }

    rs485_ctx[ch].rx_idle = false;
    rs485_ctx[ch].rx_len = 0;
    dma_disable_channel(BOARD_APP_HDMA, rs485_ctx[ch].rx_dma_chn);
    memset(rs485_dma_rx_buffer[ch], 0, RS485_DMA_BUFFER_SIZE);
    rs485_configure_dma_rx(ch);

    return writelen;
}

uint32_t rs485_rx_available(rs485_channel_t ch)
{
    if (!rs485_channel_is_valid(ch))
    {
        return 0;
    }
    (void)rs485_check_and_handle_rx(ch);
    return ring_buffer_available(&rs485_rb[ch]);
}

uint32_t rs485_rx_read(rs485_channel_t ch, uint8_t *data, uint32_t len)
{
    if (!rs485_channel_is_valid(ch) || data == NULL || len == 0)
    {
        return 0;
    }
    (void)rs485_check_and_handle_rx(ch);
    return ring_buffer_read(&rs485_rb[ch], data, len);
}

void rs485_rx_flush(rs485_channel_t ch)
{
    if (!rs485_channel_is_valid(ch))
    {
        return;
    }
    ring_buffer_reset(&rs485_rb[ch]);
    rs485_ctx[ch].rx_idle = false;
    rs485_ctx[ch].rx_len = 0;
    (void)rs485_reconfig_dma_rx(ch);
}

hpm_stat_t rs485_wait_rx_available(rs485_channel_t ch, uint32_t need_len, uint32_t timeout_ms)
{
    uint32_t start_ms;

    if (!rs485_channel_is_valid(ch) || need_len == 0)
    {
        return status_invalid_argument;
    }

    start_ms = system_get_time_ms();
    while ((uint32_t)(system_get_time_ms() - start_ms) < timeout_ms)
    {
        if (rs485_rx_available(ch) >= need_len)
        {
            return status_success;
        }
        board_delay_ms(1);
    }

    return status_timeout;
}

rs485_ctx_t *rs485_get_ctx(rs485_channel_t ch)
{
    if (ch >= RS485_CHANNEL_NUM)
    {
        return NULL;
    }
    return &rs485_ctx[ch];
}

/* 处理一帧数据的回调函数，按你的协议解析 */
static void app_parse_frame(rs485_channel_t ch, uint32_t len)
{
    uint8_t data[256];
    ring_buffer_read(&rs485_rb[ch], data, len);
    // 原函数如下
    //     if (ch == RS485_CH1) {
    //     monitor_timeout = 0;
    //     busTimeoutFlag = false;
    //     static const uint8_t reply[] = "data";
    //     rs485_send_data(ch, reply, sizeof(reply));
    // }
    if (ch == RS485_CH1)
    {
        monitor_timeout = 0;
        busTimeoutFlag = false;
        rs485_send_data(ch, data, len);
    }

    // 原本代码
    // CH2接收到数据随后转发给上位机
    else if (ch == RS485_CH2)
    {
        rs485_send_data(ch, data, len);
    }
}

void app_rs485_poll(void)
{
    uint32_t g_rx_len = 0;
    /* 第一路：外部总线监听 */
    g_rx_len = rs485_check_and_handle_rx(RS485_CH1);
    if (g_rx_len > 0)
        app_parse_frame(RS485_CH1, g_rx_len);

    /* 第二路：保留给双踏板 Modbus 控制，不在这里读取或回发 */
}

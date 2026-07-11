#ifndef _DRV_RS485_H
#define _DRV_RS485_H

#include "hpm_common.h"
#include "hpm_uart_drv.h"
#include "board.h"
#include "hpm_dmamux_drv.h"
#include "hpm_dmav2_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"
#include "hpm_clock_drv.h"
#include "drv_dma.h"

/* ============================================================
 *  RS485通道枚举
 * ============================================================ */
typedef enum {
    RS485_CH1 = 0,          /* 第一路485: RXD=PA01, TXD=PA00, DE=PA09 */
    RS485_CH2 = 1,          /* 第二路485: RXD=PB09, TXD=PB08, DE=PA10 */
    RS485_CHANNEL_NUM
} rs485_channel_t;

/* ============================================================
 *  第一路485配置
 *  RXD=PA01, TXD=PA00, DE=PA09  →  UART0
 * ============================================================ */
#define RS485_1_UART                     HPM_UART0
#define RS485_1_UART_CLK_NAME            clock_uart0
#define RS485_1_UART_IRQ                 IRQn_UART0
#define RS485_1_UART_BAUDRATE            (38400UL)

#define RS485_1_UART_TX_DMA_CHN          (21U)
#define RS485_1_UART_RX_DMA_CHN          (20U)
#define RS485_1_UART_TX_DMAMUX_CHN       DMA_SOC_CHN_TO_DMAMUX_CHN(BOARD_APP_HDMA, RS485_1_UART_TX_DMA_CHN)
#define RS485_1_UART_RX_DMAMUX_CHN       DMA_SOC_CHN_TO_DMAMUX_CHN(BOARD_APP_HDMA, RS485_1_UART_RX_DMA_CHN)
#define RS485_1_UART_TX_DMA_REQ          HPM_DMA_SRC_UART0_TX
#define RS485_1_UART_RX_DMA_REQ          HPM_DMA_SRC_UART0_RX

#define RS485_1_DE_GPIO                  HPM_GPIO0
#define RS485_1_DE_PORT                  GPIO_DO_GPIOA
#define RS485_1_DE_PIN                   (9U)

/* ============================================================
 *  第二路485配置
 *  RXD=PB09, TXD=PB08, DE=PA10  →  UART2
 * ============================================================ */
#define RS485_2_UART                     HPM_UART2
#define RS485_2_UART_CLK_NAME            clock_uart2
#define RS485_2_UART_IRQ                 IRQn_UART2
#define RS485_2_UART_BAUDRATE            (38400UL)

#define RS485_2_UART_TX_DMA_CHN          (25U)
#define RS485_2_UART_RX_DMA_CHN          (24U)
#define RS485_2_UART_TX_DMAMUX_CHN       DMA_SOC_CHN_TO_DMAMUX_CHN(BOARD_APP_HDMA, RS485_2_UART_TX_DMA_CHN)
#define RS485_2_UART_RX_DMAMUX_CHN       DMA_SOC_CHN_TO_DMAMUX_CHN(BOARD_APP_HDMA, RS485_2_UART_RX_DMA_CHN)
#define RS485_2_UART_TX_DMA_REQ          HPM_DMA_SRC_UART2_TX
#define RS485_2_UART_RX_DMA_REQ          HPM_DMA_SRC_UART2_RX

#define RS485_2_DE_GPIO                  HPM_GPIO0
#define RS485_2_DE_PORT                  GPIO_DO_GPIOA
#define RS485_2_DE_PIN                   (10U)

/* ============================================================
 *  公共配置
 * ============================================================ */
#define RS485_MODE_RX                    0
#define RS485_MODE_TX                    1

#define RS485_DMA_BUFFER_SIZE            256U
#define RS485_IDLE_THRESHOLD             40U
#define RS485_TX_TIMEOUT                 1000U

/* ============================================================
 *  RS485上下文结构体
 * ============================================================ */
typedef struct {
    UART_Type *uart;
    clock_name_t clk_name;
    uint32_t irq;
    uint32_t baudrate;
    uint8_t tx_dma_chn;
    uint8_t rx_dma_chn;
    uint32_t tx_dma_req;
    uint32_t rx_dma_req;
    GPIO_Type *de_gpio;
    uint32_t de_port;
    uint32_t de_pin;
    volatile bool tx_done;
    volatile bool rx_idle;
    uint32_t rx_len;
} rs485_ctx_t;

/* ============================================================
 *  函数声明
 * ============================================================ */
hpm_stat_t old_rs485_init(rs485_channel_t ch);
hpm_stat_t rs485_init_channel(rs485_channel_t ch, uint32_t baudrate);
void rs485_set_mode(rs485_channel_t ch, uint8_t mode);
hpm_stat_t rs485_send_data(rs485_channel_t ch, const uint8_t *data, uint32_t len);
uint32_t rs485_get_dma_received_bytes(rs485_channel_t ch);
uint32_t rs485_check_and_handle_rx(rs485_channel_t ch);
hpm_stat_t rs485_reconfig_dma_rx(rs485_channel_t ch);
uint32_t rs485_rx_available(rs485_channel_t ch);
uint32_t rs485_rx_read(rs485_channel_t ch, uint8_t *data, uint32_t len);
void rs485_rx_flush(rs485_channel_t ch);
hpm_stat_t rs485_wait_rx_available(rs485_channel_t ch, uint32_t need_len, uint32_t timeout_ms);
rs485_ctx_t *rs485_get_ctx(rs485_channel_t ch);

void rs485_1_uart_isr(void);
void rs485_2_uart_isr(void);
void app_rs485_poll(void);

#endif /* _DRV_RS485_H */

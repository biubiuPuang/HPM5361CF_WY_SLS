#ifndef _DRV_MCAN_H
#define _DRV_MCAN_H

#include "hpm_common.h"
#include "hpm_mcan_drv.h"
#include "hpm_mcan_soc.h"
#include "board.h"

/* ============================================================
 *  MCAN通道枚举
 * ============================================================ */
typedef enum {
    MCAN_CH1 = 0,          /* 第一路CAN: 监听外部总线 */
    MCAN_CH2 = 1,          /* 第二路CAN: 安全回退控制 */
    MCAN_CHANNEL_NUM
} mcan_channel_t;

/* ============================================================
 *  第一路CAN配置
 *  MCAN0
 * ============================================================ */
/* CAN配置定义 */           
#define MCAN_1_BASE                     HPM_MCAN0
#define MCAN_1_IRQ                      IRQn_MCAN0
#define MCAN_1_BAUDRATE                 (500000UL)  /* 500kbps */

/* ============================================================
 *  第二路CAN配置
 *  MCAN3
 * ============================================================ */
#define MCAN_2_BASE                     HPM_MCAN3
#define MCAN_2_IRQ                      IRQn_MCAN3
#define MCAN_2_BAUDRATE                 (500000UL)

/* ============================================================
 *  公共配置
 * ============================================================ */
#define MCAN_RX_QUEUE_SIZE              8U    /* 每路接收队列大小 */

/* ============================================================
 *  MCAN接收消息结构
 * ============================================================ */
typedef struct {
    mcan_rx_message_t msg;
    bool valid;
} mcan_rx_queue_item_t;

/* ============================================================
 *  MCAN驱动上下文
 * ============================================================ */
typedef struct {
    MCAN_Type *base;
    uint32_t clock_freq;
    uint32_t irq;
    uint32_t baudrate;
    bool initialized;
    volatile bool has_new_rx_msg;
    mcan_rx_queue_item_t rx_queue[MCAN_RX_QUEUE_SIZE];
    uint8_t rx_queue_head;
    uint8_t rx_queue_tail;
    uint8_t rx_queue_count;
} mcan_drv_context_t;

/* ============================================================
 *  函数声明
 * ============================================================ */
hpm_stat_t drv_mcan_init(mcan_channel_t ch, uint32_t baudrate);
hpm_stat_t drv_mcan_poll(mcan_channel_t ch, mcan_rx_message_t *msg);
hpm_stat_t drv_mcan_send(mcan_channel_t ch, const mcan_tx_frame_t *tx_msg);
hpm_stat_t drv_mcan_send_nonblocking(mcan_channel_t ch, const mcan_tx_frame_t *tx_msg);
mcan_drv_context_t *drv_mcan_get_ctx(mcan_channel_t ch);

void drv_mcan_1_isr(void);
void drv_mcan_2_isr(void);

#endif /* _DRV_MCAN_H */

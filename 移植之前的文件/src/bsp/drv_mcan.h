#ifndef _DRV_MCAN_H
#define _DRV_MCAN_H

#include "hpm_common.h"
#include "hpm_mcan_drv.h"
#include "hpm_mcan_soc.h"
#include "board.h"

/* MCAN配置 */
#define MCAN_RX_QUEUE_SIZE      8   /* 接收队列大小 */
#define MCAN_DEFAULT_BAUDRATE   BOARD_APP_CAN_DEFAULT_BAUDRATE

/**
 * @brief MCAN接收消息结构
 */
typedef struct {
    mcan_rx_message_t msg;
    bool valid;
} mcan_rx_queue_item_t;

/**
 * @brief MCAN驱动上下文
 */
typedef struct {
    MCAN_Type *base;
    uint32_t clock_freq;
    bool initialized;
    volatile bool has_new_rx_msg;
    mcan_rx_queue_item_t rx_queue[MCAN_RX_QUEUE_SIZE];
    uint8_t rx_queue_head;
    uint8_t rx_queue_tail;
    uint8_t rx_queue_count;
} mcan_drv_context_t;

/**
 * @brief 初始化MCAN驱动
 * 
 * @param ctx MCAN驱动上下文
 * @param baudrate 波特率
 * @return hpm_stat_t 状态码
 */
hpm_stat_t drv_mcan_init(mcan_drv_context_t *ctx, uint32_t baudrate);

/**
 * @brief MCAN中断服务程序
 */
void drv_mcan_isr(void);

/**
 * @brief 轮询接收消息（非阻塞）
 * 
 * @param ctx MCAN驱动上下文
 * @param msg 接收消息缓冲区
 * @return hpm_stat_t status_success表示有消息，status_mcan_rxfifo_empty表示无消息
 */
hpm_stat_t drv_mcan_poll(mcan_drv_context_t *ctx, mcan_rx_message_t *msg);

/**
 * @brief 发送CAN消息（阻塞）
 * 
 * @param ctx MCAN驱动上下文
 * @param tx_msg 发送消息
 * @return hpm_stat_t 状态码
 */
hpm_stat_t drv_mcan_send(mcan_drv_context_t *ctx, const mcan_tx_frame_t *tx_msg);

/**
 * @brief 发送CAN消息（非阻塞）
 * 
 * @param ctx MCAN驱动上下文
 * @param tx_msg 发送消息
 * @return hpm_stat_t 状态码
 */
hpm_stat_t drv_mcan_send_nonblocking(mcan_drv_context_t *ctx, const mcan_tx_frame_t *tx_msg);

/**
 * @brief 获取MCAN驱动上下文指针
 * 
 * @return mcan_drv_context_t* 上下文指针
 */
mcan_drv_context_t* drv_mcan_get_context(void);

#endif /* _DRV_MCAN_H */

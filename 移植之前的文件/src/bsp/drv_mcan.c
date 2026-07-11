#include "drv_mcan.h"
#include "hpm_sysctl_drv.h"
#include "hpm_mcan_soc.h"
#include "hpm_l1c_drv.h"
#include <string.h>

/* 全局MCAN驱动上下文 */
static mcan_drv_context_t s_mcan_ctx = {0};

/* MCAN中断服务程序 */
SDK_DECLARE_EXT_ISR_M(BOARD_APP_CAN_IRQn, drv_mcan_isr)
void drv_mcan_isr(void)
{
    MCAN_Type *base = BOARD_APP_CAN_BASE;
    uint32_t flags = mcan_get_interrupt_flags(base);

    /* MCAN消息RAM位于AHB_SRAM，外设写RAM时可能与DCache不一致；接收前对本实例RAM做invalidate */
    uint32_t ram_addr = mcan_get_ram_base(base) + mcan_get_ram_offset(base);
    uint32_t ram_size = mcan_get_ram_size(base);
    l1c_dc_invalidate(ram_addr, ram_size);
    
    /* 处理接收中断 */
    if ((flags & MCAN_INT_RXFIFO0_NEW_MSG) != 0) {
        mcan_rx_message_t rx_msg;
        hpm_stat_t status = mcan_read_rxfifo(base, 0, &rx_msg);
        if (status == status_success) {
            /* 调试：如需查看原始报文，可临时打开下面打印
             * printf("MCAN RX fifo0: use_ext=%u std=0x%03X ext=0x%08X dlc=%u data=%02X %02X %02X %02X ...\n",
             *        rx_msg.use_ext_id, rx_msg.std_id, rx_msg.ext_id, rx_msg.dlc,
             *        rx_msg.data_8[0], rx_msg.data_8[1], rx_msg.data_8[2], rx_msg.data_8[3]);
             */
            /* 将消息放入接收队列 */
            printf("MCAN RX fifo0: use_ext=%u std=0x%03X ext=0x%08X dlc=%u data=%02X %02X %02X %02X ...\n",
                        rx_msg.use_ext_id, rx_msg.std_id, rx_msg.ext_id, rx_msg.dlc,
                        rx_msg.data_8[0], rx_msg.data_8[1], rx_msg.data_8[2], rx_msg.data_8[3]);
            uint8_t next_tail = (s_mcan_ctx.rx_queue_tail + 1) % MCAN_RX_QUEUE_SIZE;
            if (next_tail != s_mcan_ctx.rx_queue_head) {
                /* 队列未满，可以放入 */
                memcpy(&s_mcan_ctx.rx_queue[s_mcan_ctx.rx_queue_tail].msg, &rx_msg, sizeof(mcan_rx_message_t));
                s_mcan_ctx.rx_queue[s_mcan_ctx.rx_queue_tail].valid = true;
                s_mcan_ctx.rx_queue_tail = next_tail;
                s_mcan_ctx.rx_queue_count++;
                s_mcan_ctx.has_new_rx_msg = true;
            }
        }
    }
    
    /* 处理RXFIFO1 */
    if ((flags & MCAN_INT_RXFIFO1_NEW_MSG) != 0U) {
        mcan_rx_message_t rx_msg;
        hpm_stat_t status = mcan_read_rxfifo(base, 1, &rx_msg);
        if (status == status_success) {
            uint8_t next_tail = (s_mcan_ctx.rx_queue_tail + 1) % MCAN_RX_QUEUE_SIZE;
            if (next_tail != s_mcan_ctx.rx_queue_head) {
                memcpy(&s_mcan_ctx.rx_queue[s_mcan_ctx.rx_queue_tail].msg, &rx_msg, sizeof(mcan_rx_message_t));
                s_mcan_ctx.rx_queue[s_mcan_ctx.rx_queue_tail].valid = true;
                s_mcan_ctx.rx_queue_tail = next_tail;
                s_mcan_ctx.rx_queue_count++;
                s_mcan_ctx.has_new_rx_msg = true;
            }
        }
    }
    
    /* 处理RXBUF */
    if ((flags & MCAN_INT_MSG_STORE_TO_RXBUF) != 0U) {
        /* 遍历RXBUF查找可用消息 */
        for (uint32_t buf_index = 0; buf_index < MCAN_RXBUF_SIZE_CAN_DEFAULT; buf_index++) {
            if (mcan_is_rxbuf_data_available(base, buf_index)) {
                mcan_rx_message_t rx_msg;
                hpm_stat_t status = mcan_read_rxbuf(base, buf_index, &rx_msg);
                if (status == status_success) {
                    mcan_clear_rxbuf_data_available_flag(base, buf_index);
                    uint8_t next_tail = (s_mcan_ctx.rx_queue_tail + 1) % MCAN_RX_QUEUE_SIZE;
                    if (next_tail != s_mcan_ctx.rx_queue_head) {
                        memcpy(&s_mcan_ctx.rx_queue[s_mcan_ctx.rx_queue_tail].msg, &rx_msg, sizeof(mcan_rx_message_t));
                        s_mcan_ctx.rx_queue[s_mcan_ctx.rx_queue_tail].valid = true;
                        s_mcan_ctx.rx_queue_tail = next_tail;
                        s_mcan_ctx.rx_queue_count++;
                        s_mcan_ctx.has_new_rx_msg = true;
                    }
                }
            }
        }
    }
    
    /* 清除中断标志 */
    mcan_clear_interrupt_flags(base, flags);
}

/**
 * @brief 初始化MCAN驱动
 */
hpm_stat_t drv_mcan_init(mcan_drv_context_t *ctx, uint32_t baudrate)
{
    hpm_stat_t status;
    mcan_config_t can_config;
    
    if (ctx == NULL) {
        return status_invalid_argument;
    }
    
    /* 初始化上下文 */
    memset(ctx, 0, sizeof(mcan_drv_context_t));
    ctx->base = BOARD_APP_CAN_BASE;
    
    /* 设置消息缓冲区属性（如果需要） */
#if defined(MCAN_SOC_MSG_BUF_IN_AHB_RAM) && (MCAN_SOC_MSG_BUF_IN_AHB_RAM == 1)
    /* Message RAM由SDK在 drivers/src/hpm_mcan_drv.c 中提供 mcan_soc_msg_buf[] 并放入 .ahb_sram */
#endif
    
    /* 初始化CAN引脚和时钟 */
    board_init_can(ctx->base);
    ctx->clock_freq = board_init_can_clock(ctx->base);
    
    /* 获取默认配置 */
    mcan_get_default_config(ctx->base, &can_config);
    
    /* 配置CAN参数 */
    can_config.baudrate = baudrate;
    can_config.mode = mcan_mode_normal;  /* 正常模式，非回环 */
    can_config.enable_canfd = false;     /* 禁用CANFD，使用Classic CAN */
    can_config.interrupt_mask = MCAN_EVENT_RECEIVE | MCAN_EVENT_TRANSMIT | MCAN_EVENT_ERROR;
    
    /* 配置过滤器：默认接受所有标准帧（可根据需要修改） */
    can_config.all_filters_config.global_filter_config.accept_non_matching_std_frame_option =
        MCAN_ACCEPT_NON_MATCHING_FRAME_OPTION_IN_RXFIFO0;
    can_config.all_filters_config.global_filter_config.accept_non_matching_ext_frame_option =
        MCAN_ACCEPT_NON_MATCHING_FRAME_OPTION_IN_RXFIFO1;
    
    /* 初始化MCAN */
    status = mcan_init(ctx->base, &can_config, ctx->clock_freq);
    if (status != status_success) {
        printf("MCAN: Initialization failed, error: %d\n", status);
        return status;
    }
    
    /* 使能中断 */
    mcan_enable_interrupts(ctx->base, can_config.interrupt_mask);
    intc_m_enable_irq_with_priority(BOARD_APP_CAN_IRQn, 1);
    
    ctx->initialized = true;
    s_mcan_ctx = *ctx;  /* 保存全局上下文 */
    
    printf("MCAN: Initialized successfully, baudrate: %lu bps, clock: %lu Hz\n", 
           baudrate, ctx->clock_freq);
    
    return status_success;
}

/**
 * @brief 轮询接收消息
 */
hpm_stat_t drv_mcan_poll(mcan_drv_context_t *ctx, mcan_rx_message_t *msg)
{
    if (ctx == NULL || msg == NULL) {
        return status_invalid_argument;
    }
    
    if (!ctx->initialized) {
        return status_invalid_argument;
    }
    
    /* 检查队列是否有消息 */
    if (ctx->rx_queue_count == 0) {
        return status_mcan_rxfifo_empty;
    }
    
    /* 从队列头部取出消息 */
    if (ctx->rx_queue[ctx->rx_queue_head].valid) {
        memcpy(msg, &ctx->rx_queue[ctx->rx_queue_head].msg, sizeof(mcan_rx_message_t));
        ctx->rx_queue[ctx->rx_queue_head].valid = false;
        ctx->rx_queue_head = (ctx->rx_queue_head + 1) % MCAN_RX_QUEUE_SIZE;
        ctx->rx_queue_count--;
        
        if (ctx->rx_queue_count == 0) {
            ctx->has_new_rx_msg = false;
        }
        
        return status_success;
    }
    
    return status_mcan_rxfifo_empty;
}

/**
 * @brief 发送CAN消息（阻塞）
 */
hpm_stat_t drv_mcan_send(mcan_drv_context_t *ctx, const mcan_tx_frame_t *tx_msg)
{
    if (ctx == NULL || tx_msg == NULL) {
        return status_invalid_argument;
    }
    
    if (!ctx->initialized) {
        return status_invalid_argument;
    }
    
    return mcan_transmit_blocking(ctx->base, tx_msg);
}

/**
 * @brief 发送CAN消息（非阻塞）
 */
hpm_stat_t drv_mcan_send_nonblocking(mcan_drv_context_t *ctx, const mcan_tx_frame_t *tx_msg)
{
    if (ctx == NULL || tx_msg == NULL) {
        return status_invalid_argument;
    }
    
    if (!ctx->initialized) {
        return status_invalid_argument;
    }
    
    return mcan_transmit_via_txfifo_nonblocking(ctx->base, tx_msg, NULL);
}

/**
 * @brief 获取MCAN驱动上下文指针
 */
mcan_drv_context_t* drv_mcan_get_context(void)
{
    return &s_mcan_ctx;
}

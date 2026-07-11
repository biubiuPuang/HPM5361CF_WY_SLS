#include "drv_mcan.h"
#include "hpm_sysctl_drv.h"
#include "hpm_mcan_soc.h"
#include "hpm_l1c_drv.h"
#include <string.h>

/* ============================================================
 *  全局MCAN驱动上下文（每路一个）
 * ============================================================ */
static mcan_drv_context_t s_mcan_ctx[MCAN_CHANNEL_NUM] = {
    [MCAN_CH1] = {
        .base           = MCAN_1_BASE,
        .clock_freq     = 0,
        .irq            = MCAN_1_IRQ,
        .baudrate       = MCAN_1_BAUDRATE,
        .initialized    = false,
        .has_new_rx_msg = false,
        .rx_queue_head  = 0,
        .rx_queue_tail  = 0,
        .rx_queue_count = 0,
    },
    [MCAN_CH2] = {
        .base           = MCAN_2_BASE,
        .clock_freq     = 0,
        .irq            = MCAN_2_IRQ,
        .baudrate       = MCAN_2_BAUDRATE,
        .initialized    = false,
        .has_new_rx_msg = false,
        .rx_queue_head  = 0,
        .rx_queue_tail  = 0,
        .rx_queue_count = 0,
    },
};

/* ============================================================
 *  ISR内部共用: 从指定外设读取报文并入队
 * ============================================================ */
static void mcan_isr_enqueue(mcan_drv_context_t *ctx, MCAN_Type *base, uint32_t flags)
{
    /* MCAN消息RAM位于AHB_SRAM，外设写RAM时可能与DCache不一致；接收前做invalidate */
    uint32_t ram_addr = mcan_get_ram_base(base) + mcan_get_ram_offset(base);
    uint32_t ram_size = mcan_get_ram_size(base);
    l1c_dc_invalidate(ram_addr, ram_size);

    /* RXFIFO0 */
    if ((flags & MCAN_INT_RXFIFO0_NEW_MSG) != 0) {
        mcan_rx_message_t rx_msg;
        hpm_stat_t status = mcan_read_rxfifo(base, 0, &rx_msg);
        if (status == status_success) {
            uint8_t next_tail = (ctx->rx_queue_tail + 1) % MCAN_RX_QUEUE_SIZE;
            if (next_tail != ctx->rx_queue_head) {
                memcpy(&ctx->rx_queue[ctx->rx_queue_tail].msg, &rx_msg, sizeof(mcan_rx_message_t));
                ctx->rx_queue[ctx->rx_queue_tail].valid = true;
                ctx->rx_queue_tail = next_tail;
                ctx->rx_queue_count++;
                ctx->has_new_rx_msg = true;
            }
        }
    }

    /* RXFIFO1 */
    if ((flags & MCAN_INT_RXFIFO1_NEW_MSG) != 0U) {
        mcan_rx_message_t rx_msg;
        hpm_stat_t status = mcan_read_rxfifo(base, 1, &rx_msg);
        if (status == status_success) {
            uint8_t next_tail = (ctx->rx_queue_tail + 1) % MCAN_RX_QUEUE_SIZE;
            if (next_tail != ctx->rx_queue_head) {
                memcpy(&ctx->rx_queue[ctx->rx_queue_tail].msg, &rx_msg, sizeof(mcan_rx_message_t));
                ctx->rx_queue[ctx->rx_queue_tail].valid = true;
                ctx->rx_queue_tail = next_tail;
                ctx->rx_queue_count++;
                ctx->has_new_rx_msg = true;
            }
        }
    }

    /* RXBUF */
    if ((flags & MCAN_INT_MSG_STORE_TO_RXBUF) != 0U) {
        for (uint32_t buf_index = 0; buf_index < MCAN_RXBUF_SIZE_CAN_DEFAULT; buf_index++) {
            if (mcan_is_rxbuf_data_available(base, buf_index)) {
                mcan_rx_message_t rx_msg;
                hpm_stat_t status = mcan_read_rxbuf(base, buf_index, &rx_msg);
                if (status == status_success) {
                    mcan_clear_rxbuf_data_available_flag(base, buf_index);
                    uint8_t next_tail = (ctx->rx_queue_tail + 1) % MCAN_RX_QUEUE_SIZE;
                    if (next_tail != ctx->rx_queue_head) {
                        memcpy(&ctx->rx_queue[ctx->rx_queue_tail].msg, &rx_msg, sizeof(mcan_rx_message_t));
                        ctx->rx_queue[ctx->rx_queue_tail].valid = true;
                        ctx->rx_queue_tail = next_tail;
                        ctx->rx_queue_count++;
                        ctx->has_new_rx_msg = true;
                    }
                }
            }
        }
    }

    /* 清除中断标志 */
    mcan_clear_interrupt_flags(base, flags);
}

/* ============================================================
 *  第一路CAN中断服务程序
 * ============================================================ */
void drv_mcan_1_isr(void)
{
    MCAN_Type *base = s_mcan_ctx[MCAN_CH1].base;
    uint32_t flags = mcan_get_interrupt_flags(base);
    mcan_isr_enqueue(&s_mcan_ctx[MCAN_CH1], base, flags);
}
SDK_DECLARE_EXT_ISR_M(MCAN_1_IRQ, drv_mcan_1_isr)

/* ============================================================
 *  第二路CAN中断服务程序
 * ============================================================ */
void drv_mcan_2_isr(void)
{
    MCAN_Type *base = s_mcan_ctx[MCAN_CH2].base;
    uint32_t flags = mcan_get_interrupt_flags(base);
    mcan_isr_enqueue(&s_mcan_ctx[MCAN_CH2], base, flags);
}
SDK_DECLARE_EXT_ISR_M(MCAN_2_IRQ, drv_mcan_2_isr)

/* ============================================================
 *  初始化MCAN驱动
 * ============================================================ */
hpm_stat_t drv_mcan_init(mcan_channel_t ch, uint32_t baudrate)
{
    hpm_stat_t status;
    mcan_config_t can_config;

    if (ch >= MCAN_CHANNEL_NUM) {
        return status_invalid_argument;
    }

    mcan_drv_context_t *ctx = &s_mcan_ctx[ch];

    /* 初始化CAN引脚和时钟 —— TODO: 用户自行修改board_init_can支持双通道 */
    board_init_can(ctx->base);
    ctx->clock_freq = board_init_can_clock(ctx->base);

    /* 获取默认配置 */
    mcan_get_default_config(ctx->base, &can_config);

    /* 配置CAN参数 */
    can_config.baudrate = baudrate;
    can_config.mode = mcan_mode_normal;     /* 正常模式，非回环 */
    can_config.enable_canfd = false;        /* 禁用CANFD，使用Classic CAN */
    can_config.interrupt_mask = MCAN_EVENT_RECEIVE | MCAN_EVENT_TRANSMIT | MCAN_EVENT_ERROR;

    /* 配置过滤器：默认接受所有标准帧 */
    can_config.all_filters_config.global_filter_config.accept_non_matching_std_frame_option =
        MCAN_ACCEPT_NON_MATCHING_FRAME_OPTION_IN_RXFIFO0;
    can_config.all_filters_config.global_filter_config.accept_non_matching_ext_frame_option =
        MCAN_ACCEPT_NON_MATCHING_FRAME_OPTION_IN_RXFIFO1;

    /* 初始化MCAN */
    status = mcan_init(ctx->base, &can_config, ctx->clock_freq);
    if (status != status_success) {
        printf("MCAN CH%u: Initialization failed, error: %d\n", ch, status);
        return status;
    }

    /* 使能中断 */
    mcan_enable_interrupts(ctx->base, can_config.interrupt_mask);
    intc_m_enable_irq_with_priority(ctx->irq, 1);

    ctx->initialized = true;
    ctx->baudrate = baudrate;

    printf("MCAN CH%u: Initialized successfully, baudrate: %lu bps, clock: %lu Hz\n",
           ch, baudrate, ctx->clock_freq);

    return status_success;
}

/* ============================================================
 *  轮询接收消息（非阻塞）
 * ============================================================ */
hpm_stat_t drv_mcan_poll(mcan_channel_t ch, mcan_rx_message_t *msg)
{
    if (ch >= MCAN_CHANNEL_NUM || msg == NULL) {
        return status_invalid_argument;
    }

    mcan_drv_context_t *ctx = &s_mcan_ctx[ch];

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

/* ============================================================
 *  发送CAN消息（阻塞）
 * ============================================================ */
hpm_stat_t drv_mcan_send(mcan_channel_t ch, const mcan_tx_frame_t *tx_msg)
{
    if (ch >= MCAN_CHANNEL_NUM || tx_msg == NULL) {
        return status_invalid_argument;
    }

    mcan_drv_context_t *ctx = &s_mcan_ctx[ch];

    if (!ctx->initialized) {
        return status_invalid_argument;
    }

    return mcan_transmit_blocking(ctx->base, tx_msg);
}

/* ============================================================
 *  发送CAN消息（非阻塞）
 * ============================================================ */
hpm_stat_t drv_mcan_send_nonblocking(mcan_channel_t ch, const mcan_tx_frame_t *tx_msg)
{
    if (ch >= MCAN_CHANNEL_NUM || tx_msg == NULL) {
        return status_invalid_argument;
    }

    mcan_drv_context_t *ctx = &s_mcan_ctx[ch];

    if (!ctx->initialized) {
        return status_invalid_argument;
    }

    return mcan_transmit_via_txfifo_nonblocking(ctx->base, tx_msg, NULL);
}

/* ============================================================
 *  获取MCAN驱动上下文指针
 * ============================================================ */
mcan_drv_context_t *drv_mcan_get_ctx(mcan_channel_t ch)
{
    if (ch >= MCAN_CHANNEL_NUM) {
        return NULL;
    }
    return &s_mcan_ctx[ch];
}

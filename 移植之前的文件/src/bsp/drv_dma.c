#include "drv_dma.h"
#include "board.h"
#include "hpm_interrupt.h"

/* DMA回调函数数组 */
static dma_transfer_done_callback_t dma_callbacks[DMA_SOC_CHANNEL_NUM] = {0};

/**
 * @brief 注册DMA传输完成回调函数
 */
void dma_register_callback(uint8_t channel, dma_transfer_done_callback_t callback)
{
    if (channel < DMA_SOC_CHANNEL_NUM) {
        dma_callbacks[channel] = callback;
    }
}

/**
 * @brief DMA公共中断处理函数
 */
void dma_common_isr(void)
{
    volatile hpm_stat_t status;
    
    /* 检查所有DMA通道状态 */
    for (uint8_t channel = 0; channel < DMA_SOC_CHANNEL_NUM; channel++) {
        status = dma_check_transfer_status(BOARD_APP_HDMA, channel);
        if (status & DMA_CHANNEL_STATUS_TC) {
            /* 如果有注册回调函数，则调用 */
            if (dma_callbacks[channel]) {
                dma_callbacks[channel](channel);
            }
        }
    }
}

/* 注册DMA中断处理函数 */
SDK_DECLARE_EXT_ISR_M(IRQn_HDMA, dma_common_isr) 
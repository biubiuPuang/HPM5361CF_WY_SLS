/**
 * @file drv_dma.h
 * @brief DMA驱动头文件
 */
#ifndef _DRV_DMA_H
#define _DRV_DMA_H

#include "hpm_common.h"
#include "hpm_dmav2_drv.h"

/* DMA传输完成回调函数类型 */
typedef void (*dma_transfer_done_callback_t)(uint8_t channel);

/**
 * @brief 注册DMA传输完成回调函数
 * 
 * @param channel DMA通道号
 * @param callback 回调函数
 */
void dma_register_callback(uint8_t channel, dma_transfer_done_callback_t callback);

/**
 * @brief DMA中断处理函数
 */
void dma_common_isr(void);

#endif /* _DRV_DMA_H */ 
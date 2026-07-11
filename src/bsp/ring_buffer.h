#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 *  通用环形缓冲（FIFO）
 * ------------------------------------------------------------
 *  与具体通信外设无关，UART / RS485 / SPI / CAN 等均可直接使用。
 *  采用「自由计数法」：head/tail 为累计字节数（单调递增），
 *  通过 head%size / tail%size 取实际下标，避免回绕取模错误。
 * ============================================================ */

typedef struct {
    uint8_t  *buf;
    uint32_t  size;             /* 缓冲大小（字节） */
    volatile uint32_t head;     /* 累计写入字节数 */
    volatile uint32_t tail;     /* 累计读出字节数 */
    volatile uint32_t overflow; /* 累计因缓冲满被丢弃的字节数 */
    void (*lock)(void);         /* 可选临界区锁（注入平台相关实现） */
    void (*unlock)(void);
} ring_buffer_t;

/* 初始化：传入外部缓冲和大小（buf 生命周期需 >= 本模块使用期） */
void ring_buffer_init(ring_buffer_t *rb, uint8_t *buf, uint32_t size);

/* 可选：注入临界区锁函数。多任务并发读写时建议设置；不设置则不加锁 */
void ring_buffer_set_lock(ring_buffer_t *rb, void (*lock)(void), void (*unlock)(void));

/* 写入多个字节，返回实际写入个数（缓冲不足时只写能容纳的部分，剩余记 overflow） */
uint32_t ring_buffer_write(ring_buffer_t *rb, const uint8_t *data, uint32_t len);

/* 写入单字节，成功返回 true */
bool ring_buffer_put(ring_buffer_t *rb, uint8_t c);

/* 读取多个字节，返回实际读取个数 */
uint32_t ring_buffer_read(ring_buffer_t *rb, uint8_t *data, uint32_t len);

/* 读取单字节，成功返回 true */
bool ring_buffer_get(ring_buffer_t *rb, uint8_t *c);

/* 当前可读字节数 */
uint32_t ring_buffer_available(const ring_buffer_t *rb);

/* 当前空闲字节数 */
uint32_t ring_buffer_free(const ring_buffer_t *rb);

/* 是否为空 / 是否已满 */
bool ring_buffer_is_empty(const ring_buffer_t *rb);
bool ring_buffer_is_full(const ring_buffer_t *rb);

/* 清空（仅复位读写指针，不清零 overflow 计数，也不擦除数据） */
void ring_buffer_reset(ring_buffer_t *rb);

/* 累计因缓冲满被丢弃的字节数（用于评估缓冲是否够大） */
uint32_t ring_buffer_overflow(const ring_buffer_t *rb);

#endif /* _RING_BUFFER_H */

#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb, uint8_t *buf, uint32_t size)
{
    rb->buf      = buf;
    rb->size     = size;
    rb->head     = 0;
    rb->tail     = 0;
    rb->overflow = 0;
    rb->lock     = 0;
    rb->unlock   = 0;
}

void ring_buffer_set_lock(ring_buffer_t *rb, void (*lock)(void), void (*unlock)(void))
{
    rb->lock   = lock;
    rb->unlock = unlock;
}

uint32_t ring_buffer_write(ring_buffer_t *rb, const uint8_t *data, uint32_t len)
{
    uint32_t free     = rb->size - (rb->head - rb->tail);
    uint32_t to_write = (len < free) ? len : free;

    if (rb->lock) {
        rb->lock();
    }
    for (uint32_t i = 0; i < to_write; i++) {
        rb->buf[rb->head % rb->size] = data[i];
        rb->head++;
    }
    if (rb->unlock) {
        rb->unlock();
    }

    if (to_write < len) {
        rb->overflow += (len - to_write);
    }
    return to_write;
}

bool ring_buffer_put(ring_buffer_t *rb, uint8_t c)
{
    return ring_buffer_write(rb, &c, 1) == 1;
}

uint32_t ring_buffer_read(ring_buffer_t *rb, uint8_t *data, uint32_t len)
{
    uint32_t avail    = rb->head - rb->tail;
    uint32_t to_read  = (len < avail) ? len : avail;

    if (rb->lock) {
        rb->lock();
    }
    for (uint32_t i = 0; i < to_read; i++) {
        data[i] = rb->buf[rb->tail % rb->size];
        rb->tail++;
    }
    if (rb->unlock) {
        rb->unlock();
    }

    return to_read;
}

bool ring_buffer_get(ring_buffer_t *rb, uint8_t *c)
{
    return ring_buffer_read(rb, c, 1) == 1;
}

uint32_t ring_buffer_available(const ring_buffer_t *rb)
{
    return rb->head - rb->tail;
}

uint32_t ring_buffer_free(const ring_buffer_t *rb)
{
    return rb->size - (rb->head - rb->tail);
}

bool ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}

bool ring_buffer_is_full(const ring_buffer_t *rb)
{
    return (rb->head - rb->tail) >= rb->size;
}

void ring_buffer_reset(ring_buffer_t *rb)
{
    if (rb->lock) {
        rb->lock();
    }
    rb->head = 0;
    rb->tail = 0;
    if (rb->unlock) {
        rb->unlock();
    }
    /* 注意：overflow 计数刻意保留，便于统计；如需清零可单独赋值 rb->overflow = 0 */
}

uint32_t ring_buffer_overflow(const ring_buffer_t *rb)
{
    return rb->overflow;
}

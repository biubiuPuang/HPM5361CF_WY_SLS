#ifndef PEDAL_PLATFORM_H
#define PEDAL_PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pedal_platform_sleep_ms(int ms);
uint32_t pedal_platform_now_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* PEDAL_PLATFORM_H */

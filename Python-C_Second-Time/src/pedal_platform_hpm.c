#include "pedal_platform.h"

#include "board.h"
#include "drv_timer_extra.h"

void pedal_platform_sleep_ms(int ms)
{
    if (ms > 0) {
        board_delay_ms((uint32_t)ms);
    }
}

uint32_t pedal_platform_now_ms(void)
{
    return system_get_time_ms();
}

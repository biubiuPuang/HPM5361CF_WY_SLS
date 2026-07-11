#include "pedal_platform.h"

#include <windows.h>

void pedal_platform_sleep_ms(int ms)
{
    if (ms > 0) {
        Sleep((DWORD)ms);
    }
}

uint32_t pedal_platform_now_ms(void)
{
    return (uint32_t)GetTickCount();
}

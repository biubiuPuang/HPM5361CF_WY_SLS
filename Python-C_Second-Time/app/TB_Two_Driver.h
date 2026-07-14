#ifndef TB_TWO_DRIVER__H
#define TB_TWO_DRIVER__H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// 双踏板驱动头文件
#include "pedal_controller.h"

PedalResult safety_open_pedal_controller_once(void);
bool safety_retract_pedal_once_or_retry(uint32_t now_tick);
void safety_close_pedal_controller(void);


#endif /* TB_TWO_DRIVER__H */
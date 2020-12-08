#ifndef __MICRO_SECOND_TIMER_H
#define __MICRO_SECOND_TIMER_H

#include "freertos/FreeRTOS.h"
#include "driver/timer.h"
#include <sys/types.h>
#include <functional>

#define MICRO_SECOND_TIMER_GROUP_NUM TIMER_GROUP_0
#define MICRO_SECOND_TIMER_GROUP TIMERG0
#define MICRO_SECOND_TIMER_NUM TIMER_0

class MicroSecondTimer
{
public:
    MicroSecondTimer();

    uint32_t inline IRAM_ATTR getValue()
    {
#if 1
        TIMERG0.hw_timer[TIMER_0].update = 1;
        return TIMERG0.hw_timer[TIMER_0].cnt_low;
#else
        uint64_t value;
        timer_get_counter_value(MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM, &value);
        return (uint32_t)value;
#endif
    }
};

#endif // __MICRO_SECOND_TIMER_H

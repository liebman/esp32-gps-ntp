#ifndef __MICRO_SECOND_TIMER_H
#define __MICRO_SECOND_TIMER_H

#include "freertos/FreeRTOS.h"
#include "driver/timer.h"
#include <sys/types.h>
#include <functional>

#if defined(CONFIG_GPSNTP_MICROSECOND_TIMER_GROUP_0)
#define MICRO_SECOND_TIMER_GROUP_NUM TIMER_GROUP_0
#define MICRO_SECOND_TIMER_GROUP TIMERG0
#elif defined(CONFIG_GPSNTP_MICROSECOND_TIMER_GROUP_1)
#define MICRO_SECOND_TIMER_GROUP TIMER_GROUP_1
#define MICRO_SECOND_TIMER_GROUP_VAR TIMERG1
#else
#error "no GPSNTP_MICROSECOND_TIMER_GROUP_* selected"
#endif

#if defined(CONFIG_GPSNTP_MICROSECOND_TIMER_0)
#define MICRO_SECOND_TIMER_NUM TIMER_0
#elif defined(CONFIG_GPSNTP_MICROSECOND_TIMER_1)
#define MICRO_SECOND_TIMER_NUM TIMER_1
#else
#error "no GPSNTP_MICROSECOND_TIMER_* selected"
#endif

class MicroSecondTimer
{
public:
    MicroSecondTimer();

    uint64_t inline IRAM_ATTR getValueInISR()
    {
        return timer_group_get_counter_value_in_isr(MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM);
    }

    uint64_t inline IRAM_ATTR getValue()
    {
        uint64_t value;
        timer_get_counter_value(MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM, &value);
        return value;
    }
};

#endif // __MICRO_SECOND_TIMER_H

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
    using TimeoutCB = std::function<void(void*)>;
    MicroSecondTimer();

    uint32_t inline IRAM_ATTR getValue()
    {
        // needs to be fast. I dont think concurancy will be an issue as
        // we only use the lower 32 bits!
        MICRO_SECOND_TIMER_GROUP.hw_timer[MICRO_SECOND_TIMER_NUM].update = 1;
        return MICRO_SECOND_TIMER_GROUP.hw_timer[MICRO_SECOND_TIMER_NUM].cnt_low;
    }

    void inline IRAM_ATTR reset()
    {
        MICRO_SECOND_TIMER_GROUP.hw_timer[MICRO_SECOND_TIMER_NUM].load_high = 0;
        MICRO_SECOND_TIMER_GROUP.hw_timer[MICRO_SECOND_TIMER_NUM].load_low  = 0;
        MICRO_SECOND_TIMER_GROUP.hw_timer[MICRO_SECOND_TIMER_NUM].reload = 1;
    }

    /*
     * set the timeout call back, MUSH be in set IRAM_ATTR!
    */
    void setTimeoutCB(TimeoutCB cb, void* data);
private:
    TimeoutCB _timeout_cb;
    void*     _timeout_data;
    static void timeout(void* data);
};

#endif // __MICRO_SECOND_TIMER_H

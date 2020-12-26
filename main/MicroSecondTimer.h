/*
 * MIT License
 *
 * Copyright (c) 2020 Christopher B. Liebman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

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

    uint32_t inline IRAM_ATTR getValueInISR()
    {
#if 1
        TIMERG0.hw_timer[TIMER_0].update = 1;
        return TIMERG0.hw_timer[TIMER_0].cnt_low;
#else
        return (uint32_t)timer_group_get_counter_value_in_isr(MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM);
#endif
    }

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
    uint64_t inline IRAM_ATTR getValue64()
    {
        uint64_t value;
        timer_get_counter_value(MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM, &value);
        return value;
    }
};

#endif // __MICRO_SECOND_TIMER_H

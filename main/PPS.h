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

#ifndef _PPS_H
#define _PPS_H
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "MicroSecondTimer.h"

//#define USE_LEVEL_INTERRUPT
//#define USE_INTERRUPT_SERVICE

typedef struct pps_data
{
    volatile uint32_t pps_pin;
    volatile uint32_t pps_last;
    volatile uint32_t pps_time;
    volatile uint32_t pps_min;
    volatile uint32_t pps_max;
    volatile uint32_t pps_interval;
    volatile int32_t  pps_offset;
    volatile struct pps_data* pps_ref;
    volatile uint32_t pps_short;
    volatile uint32_t pps_long;
    volatile uint32_t pps_disabled;
} pps_data_t;

class PPS
{
public:
    PPS(MicroSecondTimer& timer, pps_data_t *data, PPS* ref = nullptr);
    bool     begin(gpio_num_t pps_pin = GPIO_NUM_NC, bool expect_negedge = false);
    int      getLevel();
    time_t   getTime(struct timeval* tv);
    void     setTime(time_t time);
    uint32_t getTimerLast();
    uint32_t getTimerMin();
    uint32_t getTimerMax();
    uint32_t getTimerInterval();
    uint32_t getTimerShort();
    uint32_t getTimerLong();
    int32_t  getOffset();
    void     resetOffset();
    void     setDisable(bool disable);

protected:
    MicroSecondTimer& _timer;
    pps_data_t*       _data;
    PPS*              _ref;
    gpio_num_t         _pin           = GPIO_NUM_NC;
#ifdef USE_LEVEL_INTERRUPT
    int                 _expect_skip  = 0;
    int               _expect       = 0;
#endif
private:
    static void pps(void* data);

};

#endif // _PPS_H

#ifndef _PPS_H
#define _PPS_H
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "MicroSecondTimer.h"

class PPS
{
public:
    PPS(MicroSecondTimer& timer, gpio_num_t pps_pin = GPIO_NUM_NC, bool expect_negedge = false);
    bool  begin();
    uint32_t getCount();
    time_t   getTime(uint32_t* microseconds = nullptr);
    void     setTime(time_t time);
    void     resetMicroseconds();
    int      getLevel();
    uint64_t getLastTimer();
    uint32_t getTimerMax();
    uint32_t getTimerMin();
    uint32_t getMissed();
    uint32_t getShort();
    uint32_t getLast();
    uint32_t getHighTime();
    uint32_t getLowTime();
    uint32_t getShortLast();

protected:
    MicroSecondTimer& _timer;
    gpio_num_t  _pin;
    volatile uint64_t _last_timer;
    bool        _manage_timer;
    int         _expect_skip;
    int         _expect = 0;
    volatile uint32_t _count      = 0;
    volatile uint32_t _missed     = 0;
    volatile uint32_t _miss_last  = 0;
    volatile uint32_t _short      = 0;
    volatile uint32_t _short_last = 0;
    volatile uint32_t _timer_max  = 0;
    volatile uint32_t _timer_min  = 2000000;
    volatile uint32_t _high_time  = 0;
    volatile uint32_t _low_time   = 0;
    volatile uint32_t _last; // last pps in usecs even if short or timeout

private:
    TaskHandle_t  _task;

    static void pps(void* data);

#if 0
    TaskHandle_t  _task;
    static void ppsTask(void* data);
#endif
};

#endif // _PPS_H

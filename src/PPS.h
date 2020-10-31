#ifndef _PPS_H
#define _PPS_H
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/timer.h"

class PPS
{
public:
    PPS();
    bool     begin(gpio_num_t pps_pin = GPIO_NUM_NC, bool expect_negedge = false);
    time_t   getTime(uint32_t* microseconds);
    void     setTime(time_t time);
    uint32_t getTimerMin();
    uint32_t getTimerMax();
    uint32_t getTimerShort();
    uint32_t getTimerLong();

protected:
    gpio_num_t  _pin;
    volatile uint64_t _last_timer   = 0;
    int         _expect_skip        = 0;
    int         _expect             = 0;
    volatile uint32_t _time         = 0;
    volatile uint32_t _timer_min    = 0;
    volatile uint32_t _timer_max    = 0;
    volatile uint32_t _timer_short  = 0;
    volatile uint32_t _timer_long   = 0;

private:
    static void pps(void* data);

};

#endif // _PPS_H

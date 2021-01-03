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

#ifndef _GPS_H
#define _GPS_H
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "minmea.h"
#include <functional>
#include "PPS.h"
#include "MicroSecondTimer.h"

class GPS
{
    
public:
    explicit GPS(MicroSecondTimer& timer, uart_port_t uart_id = UART_NUM_1, size_t buffer_size = 2048);
    bool  begin(gpio_num_t tx_pin, gpio_num_t rx_pin);

    // from GSV
    int   getSatsTotal();
    // from GGA
    int   getSatsTracked();
    int   getFixQuality();
    float getAltitude();
    char  getAltitudeUnits();
    // from GSA
    char  getMode();
    int   getFixType();
    // from RMC
    bool  getValid(uint32_t max_wait_ms=10);
    uint64_t getValidSince();
    float getLatitude();
    float getLongitude();
    char* getPSTI();
    time_t getRMCTime();
    time_t getZDATime();

protected:
    MicroSecondTimer&   _timer;
    uart_port_t         _uart_id;
    size_t              _buffer_size;
    char*               _buffer = nullptr;
    // from GSV
    volatile int        _sats_total = 0;
    //from GGA
    volatile int        _sats_tracked = 0;
    volatile int        _fix_quality = 0;
    volatile float      _altitude = 0;
    volatile char       _altitude_units = 0;
    // from GSA
    volatile char       _mode = 0;
    volatile int        _fix_type = 0;
    // from RMC
    volatile bool       _valid = false;
    volatile float      _latitude = 0.0;
    volatile float      _longitude = 0.0;
    char                _psti[81] = {0};
    struct timespec     _rmc_time = {0,0};
    volatile uint64_t   _last_rmc = 0;
    volatile uint64_t   _valid_since = 0;

    // from ZDA if present
    struct timespec     _zda_time = {0,0};;

private:
    SemaphoreHandle_t   _lock;
    QueueHandle_t       _event_queue;
    TaskHandle_t        _task;

    void process(char* sentence);
    void task();
    static void task(void* data);

};

#endif // _GPS_H

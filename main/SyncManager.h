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

#ifndef _SYNC_MANAGER_H
#define _SYNC_MANAGER_H
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "GPS.h"
#include "PPS.h"
#include "DS3231.h"

class SyncManager {
public:
    SyncManager(GPS& gps, DS3231& rtc, PPS& gpspps, PPS& rtcpps);
    bool     begin();
    time_t   getGPSTime();
    time_t   getRTCTime();
    void     getRTCPPSTime(struct timeval* tv);
    void     getGPSPPSTime(struct timeval* tv);
    bool     isOffsetValid();
    float    getOffset(int32_t* minp=nullptr, int32_t* maxp = nullptr);
    float    getError();
    float    getPreviousError();
    float    getIntegral();
    uint32_t getUptime();
    float    getBias();
    void     setBias(float bias);
    float    getTarget();
    void     setTarget(float target);

    static const uint32_t PID_INTERVAL = 1;
    static const uint32_t OFFSET_DATA_SIZE = 10;

private:
    float           _Kp = 3.2;
    float           _Ki = 0.1;
    float           _Kd = 0.8;

    int32_t         _offset_data[OFFSET_DATA_SIZE] = {0};
    GPS&            _gps;
    DS3231&         _rtc;
    PPS&            _gpspps;
    PPS&            _rtcpps;
    TaskHandle_t    _task;

    //
    // Target is an offset in microseconds that we try to keep the RTC PPS from teh GPS PPS
    //
    float           _target             = 0.0;
    //
    // We can optionally bias the output to compensate for natural drift from the RTC. This
    // makes initial syncing faster as the integral does not need to build up to compensate.
    // A good value for that can be found by setting bias to zero, letting Sync match the target.
    // than take the synced integral value and multiply it by Ki.
    //
    float           _bias               = 0.0;

    volatile time_t _last_time          = 0;
    volatile time_t _rtc_time           = 0;
    time_t          _drift_start_time   = 0; // start of drift timeing (if 0 means no initial sample)
    uint32_t        _offset_index       = 0;
    uint32_t        _offset_count       = 0;
    float           _integral           = 0.0;
    float           _previous_error     = 0.0;

    void recordOffset();
    void resetOffset();
    void manageDrift(float offset);
    void process();
    void setTime(int32_t delta);
    static void task(void* data);

};

#endif // _SYNC_MANAGER_H
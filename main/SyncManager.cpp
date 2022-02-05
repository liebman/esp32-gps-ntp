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

#include "SyncManager.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#if defined(CONFIG_GPSNTP_RTC_DRIFT_MAX)
#define RTC_DRIFT_MAX CONFIG_GPSNTP_RTC_DRIFT_MAX
#else
#define RTC_DRIFT_MAX 500
#endif

#ifndef SYNC_TASK_PRI
#define SYNC_TASK_PRI configMAX_PRIORITIES
#endif

#ifndef SYNC_TASK_CORE
#define SYNC_TASK_CORE 1
#endif

#define LATENCY_PIN 2

static const char* TAG = "SyncManager";

SyncManager::SyncManager(GPS& gps, DS3231& rtc, PPS& gpspps, PPS& rtcpps)
: _gps(gps),
  _rtc(rtc),
  _gpspps(gpspps),
  _rtcpps(rtcpps)
{
}

bool SyncManager::begin()
{
    ESP_LOGI(TAG, "::begin create Sync task at priority %d core %d", SYNC_TASK_PRI, SYNC_TASK_CORE);
    xTaskCreatePinnedToCore(task, "Sync", 4096, this, SYNC_TASK_PRI, &_task, SYNC_TASK_CORE);
    return true;
}

time_t SyncManager::getGPSTime()
{
    return _gps.getRMCTime();
}

time_t SyncManager::getRTCTime()
{
    return _rtc_time;
}

void SyncManager::getRTCPPSTime(struct timeval* tv)
{
    _rtcpps.getTime(tv);
}

void SyncManager::getGPSPPSTime(struct timeval* tv)
{
    _gpspps.getTime(tv);
}

float SyncManager::getError()
{
    return _target - (float)getOffset();
}

float SyncManager::getPreviousError()
{
    return _previous_error;
}

float SyncManager::getIntegral()
{
    return _integral;
}

uint32_t SyncManager::getUptime()
{
    return (esp_timer_get_time() / 1000000);
}

void SyncManager::recordOffset()
{
    static time_t last_rtc = 0;
    static time_t last_gps = 0;
    time_t rtc_time = _rtcpps.getTime(nullptr);
    time_t gps_time = _gpspps.getTime(nullptr);

    // we want a change in both second counters before we take a new sample
    if (gps_time == last_gps || rtc_time == last_rtc)
    {
        return;
    }

    last_rtc = rtc_time;
    last_gps = gps_time;

    _offset_data[_offset_index++] = _rtcpps.getOffset();

    if (_offset_index >= OFFSET_DATA_SIZE)
    {
        _offset_index = 0;
    }

    if (_offset_count < OFFSET_DATA_SIZE)
    {
        _offset_count += 1;
    }

    uint32_t gps_interval = _gpspps.getTimerInterval();
    uint32_t rtc_interval = _rtcpps.getTimerInterval();
    if (rtc_interval < 999950 || rtc_interval > 1000050)
    {
        ESP_LOGW(TAG, "::recordOffset: RTC interval out of range: %u", rtc_interval);
    }
    if (gps_interval < 999950 || gps_interval > 1000050)
    {
        ESP_LOGW(TAG, "::recordOffset: GPS interval out of range: %u", gps_interval);
    }
}

bool SyncManager::isOffsetValid()
{
    return _offset_count == OFFSET_DATA_SIZE;
}

float SyncManager::getBias()
{
    return _bias;
}

void SyncManager::setBias(float bias)
{
    ESP_LOGI(TAG, "setBias: %0f", bias);
    _bias = bias;
    resetOffset();
}

float SyncManager::getTarget()
{
    return _target;
}

void SyncManager::setTarget(float target)
{
    ESP_LOGI(TAG, "setTarget: %0f", target);
    _target = target;
    resetOffset();
}

bool SyncManager::isValid()
{
    return _gps.getValid();
}

uint32_t SyncManager::getValidDuration()
{
    return _gps.getValidDuration();
}

uint32_t SyncManager::getValidCount()
{
    return _gps.getValidCount();
}

int8_t SyncManager::getOutput()
{
    return _output;
}

/**
 * return the average offset.  0 is returnerd if the offset data is not full.
*/
float SyncManager::getOffset(int32_t* minp, int32_t* maxp)
{
    if (_offset_count < OFFSET_DATA_SIZE)
    {
        return 0;
    }

    float total        = _offset_data[0];
    int32_t min_offset = _offset_data[0];
    int32_t max_offset = _offset_data[0];

    for(uint32_t i = 1; i < _offset_count; ++i)
    {
        total += _offset_data[i];
        if (_offset_data[i] < min_offset)
        {
            min_offset = _offset_data[i];
        }
        if (_offset_data[i] > max_offset)
        {
            max_offset = _offset_data[i];
        }
    }
    // throw out the min and max values
    total -= min_offset;
    total -= max_offset;
    float offset = total / ((float)OFFSET_DATA_SIZE-2);
    if (minp != nullptr)
    {
        *minp = min_offset;
    }
    if (maxp != nullptr)
    {
        *maxp = max_offset;
    }
    return offset;
}

void SyncManager::resetOffset()
{
    _offset_index = 0;
    _offset_count = 0;
    // reset PID controler as its invalid when we set the or finish adjusting
    _drift_start_time = 0;
    _integral = 0;
    _previous_error = 0;
}

void SyncManager::manageDrift(float offset)
{
    time_t  now = time(nullptr); // we only need simple incrementing seconds

    //
    // if drift start time is 0 then we have no initial sample take it now
    //
    if (_drift_start_time == 0)
    {
        // only initialize on a non-zero offset
        if (offset != 0)
        {
            _drift_start_time = now;
        }
        return;
    }


    uint32_t interval = now - _drift_start_time;
    if (interval >= PID_INTERVAL)
    {
        float error = _target - (float)offset;
        _integral += error;
        // limit the integral to affecting the output by 64 max (thats about 6-7 PPM)
        // Note that the integral is what builds up to compensate for any natural drift
        // in the rtc, with a ds3231 (w/temperature controled oscillator) thats a max
        // of 2ppm.  Also 64 is half the max we can adjust in either direction
        if ((_integral*_Ki) > 64.0)
        {
            _integral = 64.0/_Ki;
        }
        else if ((_integral*_Ki) < -64.0)
        {
            _integral = -64.0/_Ki;
        }
        float derivative = error - _previous_error;
        _previous_error = error;
        float output = _Kp*error + _Ki*_integral + _Kd*derivative + _bias;
        output = round(output);
        if (output > 127)
        {
            output = 127;
        }
        if (output < -127)
        {
            output = -127;
        }

        int32_t min_offset = _offset_data[0];
        int32_t max_offset = _offset_data[0];
        for (size_t i = 1; i < OFFSET_DATA_SIZE; ++i)
        {
            if (_offset_data[i] > max_offset)
            {
                max_offset = _offset_data[i];
            }
            if (_offset_data[i] < min_offset)
            {
                min_offset = _offset_data[i];
            }

        }

        if (_rtc.getAgeOffset() != (int8_t)output)
        {
            _output = output;
            _rtc.setAgeOffset((int8_t)output);
            ESP_LOGI(TAG, "::manageDrift: target=%0.1f offset=%0.1f/%d/%d error=%0.1f i=%0.1f d=%0.1f bias=%0.1f out=%d",
                    _target, offset, min_offset, max_offset, error, _integral, derivative, _bias, (int8_t)output);
        }

        _drift_start_time = now;
    }
}

void SyncManager::process()
{
    // update value of RTC display (we are the only thread allowed to talk in i2c)
    struct tm tm;
    _rtc.getTime(&tm);
    _rtc_time = mktime(&tm);

    // if the GPS is not valid then reset the offset and return
    if (!_gps.getValid())
    {
        resetOffset();
        return;
    }

    recordOffset();
    float offset = getOffset();

    struct timeval gps_tv;
    struct timeval rtc_tv;
    _gpspps.getTime(&gps_tv);
    _rtcpps.getTime(&rtc_tv);
 
    uint32_t interval = gps_tv.tv_sec - _last_time;

    // ~10 sec but only if GPS is valid and not too close to the start or end of a second!
    if (gps_tv.tv_usec > 800000
        && gps_tv.tv_usec < 900000
        && interval > 10)
    {
        // since we are almost at teh end of a second the gps message for the current sencond should have arrived
        // and we can compare it with the gps_pps second counter and update the counter if different.
        time_t gps_seconds = _gps.getRMCTime();
        if (gps_seconds != gps_tv.tv_sec)
        {
            ESP_LOGW(TAG, "updating GPS PPS Time PPS %ld -> %ld (%+ld seconds)",
                          gps_tv.tv_sec, gps_seconds, gps_seconds-gps_tv.tv_sec);
            _gpspps.setTime(gps_seconds);
            gps_tv.tv_sec = gps_seconds;
        }
        ESP_LOGV(TAG, "pps offset %0.3f", offset);
        _last_time = gps_tv.tv_sec;

        if (abs(offset) > RTC_DRIFT_MAX || gps_tv.tv_sec != rtc_tv.tv_sec)
        {
            setTime(offset);
            ESP_LOGW(TAG, "time correction happened!  PPS offset=%0.3fus gps_time=%ld rtc_time=%ld (%+ld seconds)",
                          offset, gps_tv.tv_sec, rtc_tv.tv_sec, gps_tv.tv_sec-rtc_tv.tv_sec);
            struct timeval tv;
            _rtcpps.getTime(&tv);
            settimeofday(&tv, nullptr);
            resetOffset();
        }
        return;
    }

    manageDrift(offset);
}

void SyncManager::task(void* data)
{
    ESP_LOGI(TAG, "::task - starting!");
    SyncManager* syncman = static_cast<SyncManager*>(data);
    //we dont start for a few seconds so that times can be set and initial seconds and offsets are computed

    vTaskDelay(pdMS_TO_TICKS(5000));

    while(true)
    {
        syncman->process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGE(TAG, "::task - terminating (should never happen)!");
    vTaskDelete(nullptr);
}

void SyncManager::setTime(int32_t delta)
{
    uint32_t target = 1000000 - 200; // just shy of the second mark to compensate for time to write seconds
    struct timeval tv;
    uint32_t loops = 0;
    do {
        _gpspps.getTime(&tv);
        ++loops;
    } while (tv.tv_usec < target); // busy wait for microseconds!

#ifdef SYNC_LATENCY_OUTPUT
    gpio_set_level(LATENCY_PIN, 1);
#endif

    // we are targeting the next second
    tv.tv_sec += 1; 

    // disable time incrementing in the RTC PPS so we dont accidentally increment
    // then set the RTC PPS counter time. It wil be re-enabled after the RTC has been set
    _rtcpps.setDisable(true);
    _rtcpps.setTime(tv.tv_sec);

    struct tm* tm = gmtime(&tv.tv_sec);
    if (!_rtc.setTime(tm))
    {
#ifdef SYNC_LATENCY_OUTPUT
        gpio_set_level(LATENCY_PIN, 0);
#endif
        ESP_LOGE(TAG, "setTime: failed to set time for DS3231");
        return;
    }
    _rtcpps.setDisable(false);

#ifdef SYNC_LATENCY_OUTPUT
    gpio_set_level(LATENCY_PIN, 0);
#endif

    ESP_LOGI(TAG, "setTime: success setting time! microsecond value=%ld loops=%u", tv.tv_usec, loops);
}

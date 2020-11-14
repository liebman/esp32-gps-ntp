#include "SyncManager.h"
#include "LatencyPin.h"
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
    ESP_LOGI(TAG, "::begin create GPS task at priority %d core %d", SYNC_TASK_PRI, SYNC_TASK_CORE);
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
}

/**
 * return the average offset.  0 is returnerd if the offset data is not full.
*/
int32_t SyncManager::getOffset()
{
    if (_offset_count < OFFSET_DATA_SIZE)
    {
        return 0;
    }

    int32_t total = 0;
    for(uint32_t i = 0; i < _offset_count; ++i)
    {
        total += _offset_data[i];
    }

    int32_t offset = total / (int32_t)OFFSET_DATA_SIZE;
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

void SyncManager::manageDrift(int32_t offset)
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
            ESP_LOGI(TAG, "::manageDrift: reset calculation base with start offset=%d", offset);
            _drift_start_time = now;
        }
        return;
    }

    static const float Kp = 0.60;
    static const float Ki = 0.1;
    static const float Kd = 0.5;

    uint32_t interval = now - _drift_start_time;
    if (interval >= 10)
    {
        float error = _target - (float)offset;
        _integral += error;
        if (abs(error) > 50)
        {
            _integral = 0.0;
        }
        float derivative = error - _previous_error;
        _previous_error = error;
        float output = error*Kp + Ki*_integral + Kd*derivative;
        output = round(output);
        if (output > 127)
        {
            output = 127;
        }
        if (output < -127)
        {
            output = -127;
        }
        if (_rtc.getAgeOffset() != output)
        {
            _rtc.setAgeOffset(output);
        }
        ESP_LOGI(TAG, "::manageDrift: offset=%d error=%0.1f integral=%0.3f output=%0.3f", offset, error, _integral, output);
        _drift_start_time = now;
    }
}

void SyncManager::process()
{
    // update value of RTC display (we are the only thread allowed to talk in i2c)
    struct tm tm;
    _rtc.getTime(&tm);
    _rtc_time = mktime(&tm);

    recordOffset();
    int32_t offset = getOffset();

    struct timeval gps_tv;
    struct timeval rtc_tv;
    _gpspps.getTime(&gps_tv);
    _rtcpps.getTime(&rtc_tv);
 
    uint32_t interval = gps_tv.tv_sec - _last_time;

    // ~10 sec but only if GPS is valid and not too close to the start or end of a second!
    if (gps_tv.tv_usec > 800000
        && gps_tv.tv_usec < 900000
        && _gps.getValid()
        && interval > 10)
    {
        ESP_LOGD(TAG, "pps offset %d", offset);
        _last_time = gps_tv.tv_sec;

        if (abs(offset) > RTC_DRIFT_MAX || gps_tv.tv_sec != rtc_tv.tv_sec)
        {
            setTime(offset);
            ESP_LOGI(TAG, "time correction happened!  PPS offset=%dus gps_time=%ld rtc_time%ld", offset, gps_tv.tv_sec, rtc_tv.tv_sec);
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
    SyncManager* syncman = (SyncManager*)data;
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

    ESP_LOGD(TAG, "setTime: success setting time! microsecond value=%ld loops=%u", tv.tv_usec, loops);
}

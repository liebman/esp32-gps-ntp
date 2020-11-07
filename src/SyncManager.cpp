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

int32_t SyncManager::getOffset()
{
    return _rtcpps.getOffset();
}

double SyncManager::getDrift()
{
    return _drift;
}

uint32_t SyncManager::getUptime()
{
    return (esp_timer_get_time() / 1000000);
}

void SyncManager::manageDrift()
{
    int32_t offset = _rtcpps.getOffset();
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
            _drift_start_offset = offset;
            _drift_start_time = now;
        }
        return;
    }

    // not in PPM yet
    int32_t raw_drift = _drift_start_offset - offset;
    uint32_t interval = now - _drift_start_time;

    // raw_drift of more than 10us gets us to compute drift no more than once a minute
    if (interval >= 60 && abs(raw_drift) >= 10)
    {
        _drift = (double)raw_drift / (double)interval;
        ESP_LOGI(TAG, "::manageDrift: inteval=%u raw_drift=%d drift=%0.3f", interval, raw_drift, _drift);
        _drift_start_offset = offset;
        _drift_start_time = now;
        _rtc.adjustDrift(_drift);
    }
}

void SyncManager::process()
{
    struct timeval gps_tv;
    struct timeval rtc_tv;
    _gpspps.getTime(&gps_tv);
    // ~10 sec but only if GPS is valid and not too close to the start or end of a second!
    if (gps_tv.tv_usec > 800000
        && gps_tv.tv_usec < 900000
        && _gps.getValid()
        && (gps_tv.tv_sec - _last_time) > 10)
    {
        int32_t delta = _rtcpps.getOffset();
        ESP_LOGD(TAG, "pps delta %d", delta);
        _last_time = gps_tv.tv_sec;

        _gpspps.getTime(&gps_tv);
        _rtcpps.getTime(&rtc_tv);

        if (abs(delta) > RTC_DRIFT_MAX /*|| gps_tv.tv_sec != rtc_tv.tv_sec*/)
        {
            setTime(delta);
            ESP_LOGI(TAG, "time correction happened!  PPS delta=%dus gps_time=%ld rtc_time%ld", delta, gps_tv.tv_sec, rtc_tv.tv_sec);
            struct timeval tv;
            _rtcpps.getTime(&tv);
            settimeofday(&tv, nullptr);
            _drift_start_time = 0; // reset any in-progress drift sample as its invalid when we set the time
            _rtcpps.resetOffset();
        }
    }
    else
    {
        manageDrift();
        struct tm tm;
        _rtc.getTime(&tm);
        _rtc_time = mktime(&tm);
    }
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
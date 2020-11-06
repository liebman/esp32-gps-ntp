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

        if (abs(delta) > RTC_DRIFT_MAX || gps_tv.tv_sec != rtc_tv.tv_sec)
        {
            setTime(delta);
            ESP_LOGI(TAG, "time correction happened!  PPS delta=%dus gps_time=%ld rtc_time%ld", delta, gps_tv.tv_sec, rtc_tv.tv_sec);
            struct timeval tv;
            _rtcpps.getTime(&tv);
            settimeofday(&tv, nullptr);
        }
    }
}

void SyncManager::task(void* data)
{
    ESP_LOGI(TAG, "::task - starting!");
    SyncManager* syncman = (SyncManager*)data;
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

    if (delta < 0)
    {
        tv.tv_sec += 1; // we are targeting the next second
    }

    struct tm* tm = gmtime(&tv.tv_sec);
    if (!_rtc.setTime(tm))
    {
#ifdef SYNC_LATENCY_OUTPUT
        gpio_set_level(LATENCY_PIN, 0);
#endif
        ESP_LOGE(TAG, "setTime: failed to set time for DS3231");
        return;
    }

#ifdef SYNC_LATENCY_OUTPUT
    gpio_set_level(LATENCY_PIN, 0);
#endif

    ESP_LOGD(TAG, "setTime: success setting time! microsecond value=%ld loops=%u", tv.tv_usec, loops);
}

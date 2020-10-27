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
    uint32_t microseconds;
    time_t now = _gpspps.getTime(&microseconds);
    // ~10 sec but only if GPS is valid and not too close to the start or end of a second!
    if (microseconds > 800000
        && microseconds < 900000
        && _gps.getValid()
        && (now - _last_time) > 10)
    {
        uint64_t last_rtc = _rtcpps.getLastTimer();
        uint64_t last_gps = _gpspps.getLastTimer();
        int32_t delta = last_gps - last_rtc;
        ESP_LOGD(TAG, "pps delta %d", delta);
        _last_time = now;

        time_t   pps_time = _gpspps.getTime();
        time_t   rtc_time = _rtcpps.getTime();

        if (abs(delta) > RTC_DRIFT_MAX || pps_time != rtc_time)
        {
            setTime(delta);
            ESP_LOGI(TAG, "time correction happened!  PPS delta=%dus pps_time=%ld rtc_time%ld pps_microseconds=%u", delta, pps_time, rtc_time, microseconds);
            struct timeval tv;
            tv.tv_sec = _rtcpps.getTime(&microseconds);
            tv.tv_usec = microseconds;
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
}

void SyncManager::setTime(int32_t delta)
{
    uint32_t target = 1000000 - 200; // just shy of the second mark to compensate for time to write seconds
    time_t time;
    uint32_t microseconds;
    uint32_t loops = 0;
    do {
        time = _gpspps.getTime(&microseconds);
        ++loops;
    } while (microseconds < target); // busy wait for microseconds!

#ifdef SYNC_LATENCY_OUTPUT
    gpio_set_level(LATENCY_PIN, 1);
#endif

    if (delta > 0)
    {
        time += 1; // we are targeting the next second
    }

    struct tm* tm = gmtime(&time);
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

    ESP_LOGD(TAG, "setTime: success setting time! microsecond value=%u loops=%u", microseconds, loops);
}

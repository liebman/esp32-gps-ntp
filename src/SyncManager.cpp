#include "SyncManager.h"
#include "esp_log.h"

#if defined(CONFIG_GPSNTP_RTC_DRIFT_MAX)
#define RTC_DRIFT_MAX CONFIG_GPSNTP_RTC_DRIFT_MAX
#else
#define RTC_DRIFT_MAX 500
#endif

static const char* TAG = "SyncManager";

SyncManager::SyncManager(GPS& gps, DS3231& rtc, PPS& gpspps, PPS& rtcpps)
: _gps(gps),
  _rtc(rtc),
  _gpspps(gpspps),
  _rtcpps(rtcpps)
{
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
        int32_t delta = last_rtc - last_gps;
        ESP_LOGD(TAG, "pps delta %d", delta);
        _last_time = now;

        time_t   pps_time = _gpspps.getTime();
        time_t   rtc_time = _rtcpps.getTime();

        if (abs(delta) > RTC_DRIFT_MAX || pps_time != rtc_time)
        {
            ESP_LOGI(TAG, "time correction!  PPS delta=%dus pps_time=%ld rtc_time%ld pps_microseconds=%u", delta, pps_time, rtc_time, microseconds);
            setTime(delta);
            struct timeval tv;
            tv.tv_sec = _rtcpps.getTime(&microseconds);
            tv.tv_usec = microseconds;
            settimeofday(&tv, nullptr);
        }
    }
}

void SyncManager::setTime(int32_t delta)
{
    // TODO: this is really messed up, I still get delay too long on teh first set. with value = 0 :-(

    uint32_t target = 999750; //1000000 - current; // - 200;
    time_t time;
    uint32_t microseconds;
    uint32_t loops = 0;
    do {
        time = _gpspps.getTime(&microseconds);
        ++loops;
    } while (microseconds < target); // busy wait for microseconds!

    if (delta < 0)
    {
        time += 1; // we are targeting the next second
    }

    struct tm* tm = gmtime(&time);
    if (!_rtc.setTime(tm))
    {
        ESP_LOGE(TAG, "setTime: failed to set time for DS3231");
        return;
    }

    ESP_LOGD(TAG, "setTime: success setting time! microsecond value=%u loops=%u", microseconds, loops);
}

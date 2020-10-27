#ifndef _SYNC_MANAGER_H
#define _SYNC_MANAGER_H
#include "GPS.h"
#include "PPS.h"
#include "DS3231.h"


class SyncManager {
public:
    SyncManager(GPS& gps, DS3231& rtc, PPS& gpspps, PPS& rtcpps);
    bool begin();
    void process();

private:
    GPS&            _gps;
    DS3231&         _rtc;
    PPS&            _gpspps;
    PPS&            _rtcpps;
    time_t          _last_time;
    TaskHandle_t    _task;

    void setTime(int32_t delta);
    static void task(void* data);

};

#endif // _SYNC_MANAGER_H
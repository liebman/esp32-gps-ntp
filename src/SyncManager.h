#ifndef _SYNC_MANAGER_H
#define _SYNC_MANAGER_H
#include "GPS.h"
#include "PPS.h"
#include "DS3231.h"


class SyncManager {
public:
    SyncManager(GPS& gps, DS3231& rtc, PPS& gpspps, PPS& rtcpps);
    void process();
    void setTime(int32_t delta);

private:
    GPS&    _gps;
    DS3231& _rtc;
    PPS&    _gpspps;
    PPS&    _rtcpps;
    time_t  _last_time;
};

#endif // _SYNC_MANAGER_H
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
    time_t   getRTCTime();
    void     getRTCPPSTime(struct timeval* tv);
    void     getGPSPPSTime(struct timeval* tv);
    int32_t  getOffset();
    double   getDrift();
    uint32_t getUptime();

private:
    GPS&            _gps;
    DS3231&         _rtc;
    PPS&            _gpspps;
    PPS&            _rtcpps;
    TaskHandle_t    _task;

    volatile time_t _last_time          = 0;
    volatile time_t _rtc_time           = 0;
    volatile double _drift              = 0; // in parts per million
    time_t          _drift_start_time   = 0; // start of drift timeing (if 0 means no initial sample)
    int32_t         _drift_start_offset = 0; // initial drift offset sample
    time_t          _last_manage_drift  = 0;
    uint32_t        _offset_index        = 0;
    uint32_t        _offset_count        = 0;
    static const uint32_t OFFSET_DATA_SIZE = 10;
    int32_t         _offset_data[OFFSET_DATA_SIZE];

    void recordOffset();
    void resetOffset();
    void manageDrift(int32_t offset);
    void process();
    void setTime(int32_t delta);
    static void task(void* data);

};

#endif // _SYNC_MANAGER_H
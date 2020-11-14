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
    int32_t  getOffset();
    float    getError();
    float    getIntegral();
    uint32_t getUptime();

private:
    static const uint32_t OFFSET_DATA_SIZE = 10;
    int32_t            _offset_data[OFFSET_DATA_SIZE];
    GPS&               _gps;
    DS3231&            _rtc;
    PPS&               _gpspps;
    PPS&               _rtcpps;
    TaskHandle_t       _task;

    volatile time_t    _last_time          = 0;
    volatile time_t    _rtc_time           = 0;
    time_t             _drift_start_time   = 0; // start of drift timeing (if 0 means no initial sample)
    uint32_t           _offset_index       = 0;
    uint32_t           _offset_count       = 0;
    float              _integral           = 0.0;
    float              _previous_error     = 0.0;

    void recordOffset();
    void resetOffset();
    void manageDrift(int32_t offset);
    void process();
    void setTime(int32_t delta);
    static void task(void* data);

};

#endif // _SYNC_MANAGER_H
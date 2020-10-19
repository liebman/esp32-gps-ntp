#ifndef _PAGE_GPS_H_
#define _PAGE_GPS_H_

#include "GPS.h"
#include "PPS.h"
#include "LVPage.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PageGPS {
public:
    PageGPS(GPS& gps, PPS& pps, PPS& rtcpps);
    ~PageGPS();
private:
    void update();
    static void task(lv_task_t* task);
    GPS&     _gps;
    PPS&     _pps;
    PPS&     _rtcpps;
    LVPage*  _page;
    LVLabel* _sats;
    LVLabel* _status;
    LVLabel* _pos;
    LVLabel* _rmc_time;
    LVLabel* _ppsl;
    LVLabel* _rtcppsl;
    LVLabel* _psti;

    LVStyle  _container_style;
};

#endif // _PAGE_GPS_H_

#ifndef _PAGE_GPS_H_
#define _PAGE_GPS_H_

#include "GPS.h"
#include "LVPage.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PageGPS {
public:
    PageGPS(GPS& gps);
    ~PageGPS();
private:
    void update();
    static void task(lv_task_t* task);
    GPS&     _gps;
    LVPage*  _page;
    LVLabel* _sats;
    LVLabel* _status;
    LVLabel* _pos;
    LVLabel* _rmc_time;
    LVLabel* _pps;
    LVLabel* _psti;

    LVStyle  _container_style;
};

#endif // _PAGE_GPS_H_

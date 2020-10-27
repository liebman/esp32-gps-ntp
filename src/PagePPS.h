#ifndef _PAGE_PPS_H_
#define _PAGE_PPS_H_

#include "PPS.h"
#include "LVPage.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PagePPS {
public:
    PagePPS(PPS& gpspps, PPS& rtcpps);
    ~PagePPS();
private:
    void update();
    static void task(lv_task_t* task);
    PPS&     _gpspps;
    PPS&     _rtcpps;

    LVPage*  _page;
    LVLabel* _sys_time;
    LVLabel* _gps_time;
    LVLabel* _rtc_time;
    LVLabel* _delta;
    LVStyle  _container_style;
};

#endif // _PAGE_GPS_H_

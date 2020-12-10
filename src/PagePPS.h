#ifndef _PAGE_PPS_H_
#define _PAGE_PPS_H_

#include "PPS.h"
#include "LVPage.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PagePPS {
public:
    PagePPS(PPS& gps_pps, PPS& rtc_pps);
    ~PagePPS();
private:
    void update();
    static void task(lv_task_t* task);
    PPS&     _gps_pps;
    PPS&     _rtc_pps;

    LVPage*  _page;
    LVLabel* _gps_time;
    LVLabel* _rtc_time;
    LVLabel* _gps_interval;
    LVLabel* _rtc_interval;
    LVLabel* _gps_minmax;
    LVLabel* _gps_shortlong;
    LVLabel* _rtc_minmax;
    LVLabel* _rtc_shortlong;
    LVLabel* _rtc_offset;
    LVStyle  _container_style;
};

#endif // _PAGE_GPS_H_

#ifndef _PAGE_PPS_H_
#define _PAGE_PPS_H_

#include "PPS.h"
#include "LVPage.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PagePPS {
public:
    PagePPS(PPS& gps_pps);
    ~PagePPS();
private:
    void update();
    static void task(lv_task_t* task);
    PPS&     _gps_pps;

    LVPage*  _page;
    LVLabel* _gps_time;
    LVStyle  _container_style;
};

#endif // _PAGE_GPS_H_

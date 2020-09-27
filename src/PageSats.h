#ifndef _PAGE_SATS_H_
#define _PAGE_SATS_H_

#include "GPS.h"
#include "LVPage.h"
#include "LVChart.h"
#include "LVStyle.h"

class PageSats {
public:
    PageSats(GPS& gps);
    ~PageSats();
private:
    void update();
    static void task(lv_task_t* task);
    GPS&     _gps;
    LVPage*  _page;
    LVChart* _chart;
    lv_chart_series_t* _tracked;
    LVStyle  _container_style;
    LVStyle  _chart_style;
};

#endif // _PAGE_SATS_H_

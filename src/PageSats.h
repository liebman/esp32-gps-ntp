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
    lv_chart_series_t* _tracked_max_series;
    lv_chart_series_t* _tracked_min_series;
    LVStyle  _container_style;
    LVStyle  _chart_style;

    uint16_t _tracked_max = 0;
    uint16_t _tracked_min = 0;

    time_t _last_time = 0;
};

#endif // _PAGE_SATS_H_

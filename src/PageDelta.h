#ifndef _PAGE_DELTA_H_
#define _PAGE_DELTA_H_

#include "SyncManager.h"
#include "LVPage.h"
#include "LVChart.h"
#include "LVStyle.h"

class PageDelta {
public:
    PageDelta(SyncManager& syncman);
    ~PageDelta();
private:
    void update();
    static void task(lv_task_t* task);
    SyncManager&  _syncman;
    LVPage*  _page;
    LVChart* _chart;
    lv_chart_series_t* _offset_series;
    LVStyle  _container_style;
    LVStyle  _chart_style;

    time_t _last_time = 0;
};

#endif // _PAGE_DELTA_H_

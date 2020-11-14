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
    lv_chart_series_t* _min_error_series;
    lv_chart_series_t* _max_error_series;
    lv_chart_series_t* _avg_error_series;
    LVStyle  _container_style;
    LVStyle  _chart_style;

    int32_t _total_error = 0;
    int32_t _total_count = 0;
    int32_t _min_error = 0;
    int32_t _max_error = 0;
    time_t _last_time = 0;
};

#endif // _PAGE_DELTA_H_

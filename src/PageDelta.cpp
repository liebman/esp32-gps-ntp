#include "PageDelta.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageDelta";

PageDelta::PageDelta(SyncManager& syncman) : _syncman(syncman)
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        //_chart_style.setRadius(LV_STATE_DEFAULT, 0);
        _chart_style.setSize(LV_STATE_DEFAULT, 1);
        _chart_style.setLineWidth(LV_STATE_DEFAULT, 1);

        _page = Display::getDisplay().newPage("Delta");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_MID);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _chart = new LVChart(cont);
        _chart->addStyle(LV_CHART_PART_SERIES, &_chart_style);
        _chart->setSize(315, 200);
        _chart->setType(LV_CHART_TYPE_LINE);
        _chart->setDragParent(true);
        _chart->setPointCount(60);
        _chart->setDivLineCount(3, 0);
        _chart->setYRange(LV_CHART_AXIS_PRIMARY_Y, -10, 10);
        _chart->setYTickLength(10, 5);
        _chart->setYTickTexts("-10\n-5\n0\n5\n10", 5, LV_CHART_AXIS_DRAW_LAST_TICK|LV_CHART_AXIS_INVERSE_LABELS_ORDER);
        _chart->setStylePadLeft(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 40);
        _chart->setStyleBorderWidth(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);


        _min_error_series = _chart->addSeries(LV_COLOR_RED);
        _max_error_series = _chart->addSeries(LV_COLOR_RED);
        _avg_error_series = _chart->addSeries(LV_COLOR_GREEN);

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 1000, LV_TASK_PRIO_LOW, this);
    });
}

PageDelta::~PageDelta()
{
}

void PageDelta::task(lv_task_t *task)
{
    PageDelta* p = (PageDelta*)task->user_data;
    p->update();
}

void PageDelta::update()
{
    time_t now = time(nullptr);
    int32_t error = _syncman.getError();
    if (error < _min_error || _total_count == 0)
    {
        _min_error = error;
    }
    if (error > _max_error || _total_count == 0)
    {
        _max_error = error;
    }

    _total_error += error;
    _total_count += 1;

    if ((now - _last_time) >= 60)
    {
        _chart->setNext(_min_error_series, _min_error);
        _chart->setNext(_max_error_series, _max_error);
        int avg = 0;
        if (_total_count > 0)
        {
            avg = _total_error / _total_count;
        }
        _chart->setNext(_avg_error_series, avg);
        _min_error = 0;
        _max_error = 0;
        _total_count = 0;
        _total_error = 0;
        _last_time = now;
    }
}

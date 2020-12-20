#include "PageSats.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageSats";

PageSats::PageSats(GPS& gps) : _gps(gps)
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

        _page = Display::getDisplay().newPage("Sats");
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
        _chart->setDivLineCount(4, 0);
        _chart->setYRange(LV_CHART_AXIS_PRIMARY_Y, 0, 25);
        _chart->setYTickLength(10, 5);
        _chart->setYTickTexts("0\n5\n10\n15\n20\n25", 5, LV_CHART_AXIS_DRAW_LAST_TICK|LV_CHART_AXIS_INVERSE_LABELS_ORDER);
        _chart->setStylePadLeft(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 35);
        _chart->setStyleBorderWidth(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);

        _tracked_max_series = _chart->addSeries(LV_COLOR_LIME);
        _tracked_min_series = _chart->addSeries(LV_COLOR_ORANGE);

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 500, LV_TASK_PRIO_LOW, this);
    });
}

PageSats::~PageSats()
{
}

void PageSats::task(lv_task_t *task)
{
    PageSats* p = (PageSats*)task->user_data;
    p->update();
}

void PageSats::update()
{
    int tracked = _gps.getSatsTracked();
    if (tracked > _tracked_max)
    {
        _tracked_max = tracked;
    }
    if (tracked < _tracked_min)
    {
        _tracked_min = tracked;
    }

    time_t now = time(nullptr);
    // update the chart once a minute
    if (now > (_last_time + 60))
    {
        _chart->setNext(_tracked_min_series, _tracked_min);
        _chart->setNext(_tracked_max_series, _tracked_max);
        _tracked_min = tracked;
        _tracked_max = tracked;
        _last_time = now;
    }
}

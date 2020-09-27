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

        _chart_style.setRadius(LV_STATE_DEFAULT, 0);
        _chart_style.setSize(LV_STATE_DEFAULT, 1);
        _chart_style.setLineWidth(LV_STATE_DEFAULT, 1);

        _page = Display::getDisplay().newPage("Sats");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_LEFT);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _chart = new LVChart(cont);
        _chart->addStyle(LV_CHART_PART_SERIES, &_chart_style);
        _chart->setSize(300, 200);
        _chart->setType(LV_CHART_TYPE_LINE);
        _chart->setDragParent(true);
        _chart->setPointCount(120);
        _chart->setYRange(LV_CHART_AXIS_PRIMARY_Y, 0, 25);
        _tracked = _chart->addSeries(LV_COLOR_RED);

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 1000, LV_TASK_PRIO_LOW, this);
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
    _chart->setNext(_tracked, _gps.getSatsTracked());
}

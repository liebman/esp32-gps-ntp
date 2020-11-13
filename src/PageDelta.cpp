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
        _chart->setPointCount(90);
        _chart->setDivLineCount(7, 0);
        _chart->setYRange(LV_CHART_AXIS_PRIMARY_Y, -10, 30);
        _chart->setYTickLength(10, 5);
        _chart->setYTickTexts("-10\n-5\n0\n5\n10\n15\n20\n25\n30", 5, LV_CHART_AXIS_DRAW_LAST_TICK|LV_CHART_AXIS_INVERSE_LABELS_ORDER);
        _chart->setStylePadLeft(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 40);
        _chart->setStyleBorderWidth(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);

        _offset_series = _chart->addSeries(LV_COLOR_BLACK);
        update();

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 10000, LV_TASK_PRIO_LOW, this);
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

#if 0
static int16_t roundUpTo5(int16_t numToRound)
{
    int remainder = abs(numToRound) % 5;

    if (remainder == 0)
        return numToRound;

    if (numToRound < 0)
    {
        return numToRound + remainder;
    }

    return numToRound + (5 - remainder);
}

static int16_t roundDownTo5(int16_t numToRound)
{
    int remainder = abs(numToRound) % 5;

    if (remainder == 0)
        return numToRound;

    if (numToRound < 0)
    {
        return numToRound - (5 - remainder);
    }

    return numToRound = remainder;
}
#endif

void PageDelta::update()
{
    int32_t offset = _syncman.getOffset();
    _chart->setNext(_offset_series, offset);
#if 0
    // find the series max and min.
    int16_t max_offset = offset;
    int16_t min_offset = offset;
    for (uint16_t i = 0; i < _chart->getPointCount(); ++i)
    {
        lv_coord_t val = _chart->getPointId(_offset_series, i);
        if (val > max_offset)
        {
            max_offset = val;
        }
        if (val < min_offset)
        {
            min_offset = val;
        }
    }

    lv_coord_t max_scale = roundUpTo5(max_offset);
    lv_coord_t min_scale = roundDownTo5(min_offset);

    if (max_scale == min_scale)
    {
        max_scale = max_scale + 5;
        min_scale = min_scale - 5;
    }

    ESP_LOGI(TAG, "::update range: %d -> %d rounded: %d -> %d", min_offset, max_offset, min_scale, max_scale);
    _chart->setYRange(LV_CHART_AXIS_PRIMARY_Y, min_scale, max_scale);
#endif
}

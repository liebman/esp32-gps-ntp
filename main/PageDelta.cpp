/*
 * MIT License
 *
 * Copyright (c) 2020 Christopher B. Liebman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

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

        _page = Display::getDisplay().newPage("Dlt");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_MID);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _overview_chart = new LVChart(cont);
        _overview_chart->addStyle(LV_CHART_PART_SERIES, &_chart_style);
        _overview_chart->setSize(315, 100);
        _overview_chart->setType(LV_CHART_TYPE_LINE);
        _overview_chart->setDragParent(true);
        _overview_chart->setPointCount(60);
        _overview_chart->setDivLineCount(3, 0);
        _overview_chart->setYRange(LV_CHART_AXIS_PRIMARY_Y, -10, 10);
        _overview_chart->setYTickLength(10, 5);
        _overview_chart->setYTickTexts("-10\n-5\n0\n5\n10", 5, LV_CHART_AXIS_DRAW_LAST_TICK|LV_CHART_AXIS_INVERSE_LABELS_ORDER);
        _overview_chart->setStylePadLeft(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 40);
        _overview_chart->setStyleBorderWidth(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);

        _min_error_series = _overview_chart->addSeries(LV_COLOR_RED);
        _max_error_series = _overview_chart->addSeries(LV_COLOR_RED);
        _avg_error_series = _overview_chart->addSeries(LV_COLOR_GREEN);

        _detail_chart = new LVChart(cont);
        _detail_chart->addStyle(LV_CHART_PART_SERIES, &_chart_style);
        _detail_chart->setSize(315, 100);
        _detail_chart->setType(LV_CHART_TYPE_LINE);
        _detail_chart->setDragParent(true);
        _detail_chart->setPointCount(60);
        _detail_chart->setDivLineCount(3, 0);
        _detail_chart->setYRange(LV_CHART_AXIS_PRIMARY_Y, -10, 10);
        _detail_chart->setYTickLength(10, 5);
        _detail_chart->setYTickTexts("-10\n-5\n0\n5\n10", 5, LV_CHART_AXIS_DRAW_LAST_TICK|LV_CHART_AXIS_INVERSE_LABELS_ORDER);
        _detail_chart->setStylePadLeft(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 40);
        _detail_chart->setStyleBorderWidth(LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);

        _error_series = _detail_chart->addSeries(LV_COLOR_BLACK);

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, SyncManager::OFFSET_DATA_SIZE*1000, LV_TASK_PRIO_LOW, this);
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
    if (!_syncman.isOffsetValid())
    {
        return;
    }

    time_t now = time(nullptr);
    int32_t error = round(_syncman.getError());

    _detail_chart->setNext(_error_series, error);

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

    if ((now - _last_time) >= (60*30)) // every 30 minutes giving 30 hours on the graph
    {
        _overview_chart->setNext(_min_error_series, _min_error);
        _overview_chart->setNext(_max_error_series, _max_error);
        int avg = 0;
        if (_total_count > 0)
        {
            avg = _total_error / _total_count;
        }
        _overview_chart->setNext(_avg_error_series, avg);
        _min_error = 0;
        _max_error = 0;
        _total_count = 0;
        _total_error = 0;
        _last_time = now;
    }
}

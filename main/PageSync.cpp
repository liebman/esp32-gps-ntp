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

#include "PageSync.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageSync";

enum Row
{
    RTC_TIME = 0,
    GPS_TIME,
    RTC_PPS_TIME,
    GPS_PPS_TIME,
    OFFSET,
    ERROR,
    INTEGRAL,
    _NUM_ROWS
};

PageSync::PageSync(SyncManager& syncman)
: _syncman(syncman)
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        _page = Display::getDisplay().newPage("Syn");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_LEFT);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _table = new LVTable(cont);
        _table->addStyle(LV_TABLE_PART_BG, &_container_style);
        _table->addStyle(LV_TABLE_PART_CELL1, &_container_style);
        _table->setColumnCount(2);
        _table->setColumnWidth(0, 80);
        _table->setColumnWidth(1, 160);
        _table->setRowCount(Row::_NUM_ROWS);
        _table->setCellAlign(Row::RTC_TIME, 0, LV_LABEL_ALIGN_RIGHT);
        _table->setCellAlign(Row::GPS_TIME, 0, LV_LABEL_ALIGN_RIGHT);
        _table->setCellAlign(Row::RTC_PPS_TIME, 0, LV_LABEL_ALIGN_RIGHT);
        _table->setCellAlign(Row::GPS_PPS_TIME, 0, LV_LABEL_ALIGN_RIGHT);
        _table->setCellAlign(Row::OFFSET, 0, LV_LABEL_ALIGN_RIGHT);
        _table->setCellAlign(Row::ERROR, 0, LV_LABEL_ALIGN_RIGHT);
        _table->setCellAlign(Row::INTEGRAL, 0, LV_LABEL_ALIGN_RIGHT);

        _table->setCellValue(Row::RTC_TIME, 0, "RTC:");
        _table->setCellValue(Row::GPS_TIME, 0, "GPS:");
        _table->setCellValue(Row::RTC_PPS_TIME, 0, "RTC PPS:");
        _table->setCellValue(Row::GPS_PPS_TIME, 0, "GPS PPS:");
        _table->setCellValue(Row::OFFSET, 0, "Offset:");
        _table->setCellValue(Row::ERROR, 0, "Error:");
        _table->setCellValue(Row::INTEGRAL, 0, "Integral:");

        _table->setCellAlign(Row::RTC_TIME, 1, LV_LABEL_ALIGN_LEFT);
        _table->setCellAlign(Row::GPS_TIME, 1, LV_LABEL_ALIGN_LEFT);
        _table->setCellAlign(Row::RTC_PPS_TIME, 1, LV_LABEL_ALIGN_LEFT);
        _table->setCellAlign(Row::GPS_PPS_TIME, 1, LV_LABEL_ALIGN_LEFT);
        _table->setCellAlign(Row::OFFSET, 1, LV_LABEL_ALIGN_LEFT);
        _table->setCellAlign(Row::ERROR, 1, LV_LABEL_ALIGN_LEFT);
        _table->setCellAlign(Row::INTEGRAL, 1, LV_LABEL_ALIGN_LEFT);

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 100, LV_TASK_PRIO_LOW, this);
    });
}

PageSync::~PageSync()
{
}

void PageSync::task(lv_task_t *task)
{
    PageSync* p = static_cast<PageSync*>(task->user_data);
    p->update();
}

static void fmtTime(const char* label, char* result, const size_t size, const time_t time, const uint32_t microseconds)
{
    char buf[256];
    struct tm tm;
    gmtime_r(&time, &tm);
    strftime(buf, sizeof(buf)-1, "%Y:%m:%d %H:%M:%S", &tm);
    if (microseconds < 1000000)
    {
        snprintf(result, size, "%s%s.%02u", label, buf, microseconds/10000);
    }
    else
    {
        strncpy(result, buf, size);
    }
}

void PageSync::update()
{
    static char buf[128];
    struct timeval tv;
    time_t now = _syncman.getRTCTime();
    fmtTime("", buf, sizeof(buf)-1, now, 0xffffffff);
    _table->setCellValue(Row::RTC_TIME, 1, buf);

    now = _syncman.getGPSTime();
    fmtTime("", buf, sizeof(buf)-1, now, 0xffffffff);
    _table->setCellValue(Row::GPS_TIME, 1, buf);

    _syncman.getRTCPPSTime(&tv);
    fmtTime("", buf, sizeof(buf)-1, tv.tv_sec, tv.tv_usec);
    _table->setCellValue(Row::RTC_PPS_TIME, 1, buf);

    _syncman.getGPSPPSTime(&tv);
    fmtTime("", buf, sizeof(buf)-1, tv.tv_sec, tv.tv_usec);
    _table->setCellValue(Row::GPS_PPS_TIME, 1, buf);

    snprintf(buf, sizeof(buf)-1, "%0.3f", _syncman.getOffset());
    _table->setCellValue(Row::OFFSET, 1, buf);

    snprintf(buf, sizeof(buf)-1, "%0.3f", _syncman.getPreviousError());
    _table->setCellValue(Row::ERROR, 1, buf);

    snprintf(buf, sizeof(buf)-1, "%0.3f", _syncman.getIntegral());
    _table->setCellValue(Row::INTEGRAL, 1, buf);
}

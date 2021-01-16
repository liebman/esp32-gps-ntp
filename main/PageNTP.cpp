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

#include "PageNTP.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageNTP";

enum Row
{
    REQUESTS,
    RESPONSES,
    UPTIME,
    VALIDTIME,
    VALIDCOUNT,
    _NUM_ROWS
};

static const char* labels[_NUM_ROWS] = {"Req:", "Resp:", "Uptime:", "Valid:", "ValidCount:"};

PageNTP::PageNTP(NTP& ntp, SyncManager& syncman)
: _ntp(ntp),
  _syncman(syncman)
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        _page = Display::getDisplay().newPage("NTP");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_LEFT);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _datetime = new LVLabel(cont);
        _datetime->setStyleTextFont(LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_28);

        _table = new LVTable(cont);
        _table->addStyle(LV_TABLE_PART_BG, &_container_style);
        _table->addStyle(LV_TABLE_PART_CELL1, &_container_style);
        _table->setColumnCount(2);
        _table->setColumnWidth(0, 85);
        _table->setColumnWidth(1, 160);
        _table->setRowCount(Row::_NUM_ROWS);

        for (int row = 0; row < Row::_NUM_ROWS; ++row)
        {
            _table->setCellAlign(row, 0, LV_LABEL_ALIGN_RIGHT);
            _table->setCellValue(row, 0, labels[row]);
            _table->setCellAlign(row, 1, LV_LABEL_ALIGN_LEFT);
        }

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 100, LV_TASK_PRIO_LOW, this);
    });
}

PageNTP::~PageNTP()
{
}

void PageNTP::task(lv_task_t *task)
{
    PageNTP* p = static_cast<PageNTP*>(task->user_data);
    p->update();
}

static void fmtTime(const char* label, char* result, const size_t size, const time_t time, const uint32_t microseconds)
{
    char buf[256];
    struct tm tm;
    gmtime_r(&time, &tm);
    strftime(buf, sizeof(buf)-1, "%Y/%m/%d %H:%M:%S", &tm);
    if (microseconds < 1000000)
    {
        snprintf(result, size, "%s%s.%02u", label, buf, microseconds/10000);
    }
    else
    {
        strncpy(result, buf, size);
    }
}

static void duration(char* buf, size_t size, uint32_t seconds)
{
    uint32_t days     = seconds / 86400;
    seconds          -= days * 86400;
    uint32_t hours    = seconds /3600;
    seconds          -= hours * 3600;
    uint32_t minutes  = seconds / 60;
    seconds          -= minutes * 60;
    snprintf(buf, size-1, "%ud %02uh %02um %02us", days, hours, minutes, seconds);
}

void PageNTP::update()
{
    static char buf[128];
    struct timeval tv;
    _syncman.getRTCPPSTime(&tv);
    fmtTime(" ", _time_buf, sizeof(buf)-1, tv.tv_sec, tv.tv_usec);
    _datetime->setText(_time_buf);
    _datetime->setStyleTextColor(LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, _syncman.isValid() ? LV_COLOR_LIME : LV_COLOR_RED);

    snprintf(buf, sizeof(buf)-1, "%u", _ntp.getRequests());
    _table->setCellValue(Row::REQUESTS, 1, buf);

    snprintf(buf, sizeof(buf)-1, "%u", _ntp.getResponses());
    _table->setCellValue(Row::RESPONSES, 1, buf);

    uint32_t seconds = esp_timer_get_time() / 1000000; // uptime in seconds
    duration(buf, sizeof(buf), seconds);
    _table->setCellValue(Row::UPTIME, 1, buf);

    duration(buf, sizeof(buf)-1, _syncman.getValidDuration());
    _table->setCellValue(Row::VALIDTIME, 1, buf);

    snprintf(buf, sizeof(buf)-1, "%u", _syncman.getValidCount());
    _table->setCellValue(Row::VALIDCOUNT, 1, buf);
}

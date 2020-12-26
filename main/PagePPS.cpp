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

#include "PagePPS.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PagePPS";

PagePPS::PagePPS(PPS& gps_pps, PPS& rtc_pps)
: _gps_pps(gps_pps),
  _rtc_pps(rtc_pps)
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        _page = Display::getDisplay().newPage("PPS");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_LEFT);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _gps_time      = new LVLabel(cont);
        _rtc_time      = new LVLabel(cont);
        _gps_interval  = new LVLabel(cont);
        _gps_minmax    = new LVLabel(cont);
        _gps_shortlong = new LVLabel(cont);
        _rtc_interval  = new LVLabel(cont);
        _rtc_minmax    = new LVLabel(cont);
        _rtc_shortlong = new LVLabel(cont);
        _rtc_offset    = new LVLabel(cont);

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 100, LV_TASK_PRIO_LOW, this);
    });
}

PagePPS::~PagePPS()
{
}

void PagePPS::task(lv_task_t *task)
{
    PagePPS* p = (PagePPS*)task->user_data;
    p->update();
}

static void fmtTime(const char* label, char* result, const size_t size, const time_t time, const uint32_t microseconds)
{
    char buf[256];
    struct tm tm;
    gmtime_r(&time, &tm);
    strftime(buf, sizeof(buf)-1, "%Y:%m:%d %H:%M:%S", &tm);
    snprintf(result, size, "%s%s.%02u", label, buf, microseconds/10000);
}

void PagePPS::update()
{
    static char buf[128];
    struct timeval tv;
    _gps_pps.getTime(&tv);
    fmtTime("GPS PPS: ", buf, sizeof(buf)-1, tv.tv_sec, tv.tv_usec);
    _gps_time->setText(buf);

    _rtc_pps.getTime(&tv);
    fmtTime("RTC PPS: ", buf, sizeof(buf)-1, tv.tv_sec, tv.tv_usec);
    _rtc_time->setText(buf);

    snprintf(buf, sizeof(buf)-1, "GPS Interval: %07u", _gps_pps.getTimerInterval());
    _gps_interval->setText(buf);

    snprintf(buf, sizeof(buf)-1, "GPS Min/Max: %06u / %07u",
            _gps_pps.getTimerMin(),_gps_pps.getTimerMax());
    _gps_minmax->setText(buf);

    snprintf(buf, sizeof(buf)-1, "Short/Long: %u / %u",
            _gps_pps.getTimerShort(), _gps_pps.getTimerLong());
    _gps_shortlong->setText(buf);

    snprintf(buf, sizeof(buf)-1, "RTC Interval: %07u", _rtc_pps.getTimerInterval());
    _rtc_interval->setText(buf);

    snprintf(buf, sizeof(buf)-1, "RTC Min/Max: %06u / %07u",
            _rtc_pps.getTimerMin(),_rtc_pps.getTimerMax());
    _rtc_minmax->setText(buf);

    snprintf(buf, sizeof(buf)-1, "Short/Long: %u / %u",
            _rtc_pps.getTimerShort(), _rtc_pps.getTimerLong());
    _rtc_shortlong->setText(buf);

    snprintf(buf, sizeof(buf)-1, "RTC Offset: %d", _rtc_pps.getOffset());
    _rtc_offset->setText(buf);

}

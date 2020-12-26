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

#ifndef _PAGE_SATS_H_
#define _PAGE_SATS_H_

#include "GPS.h"
#include "LVPage.h"
#include "LVChart.h"
#include "LVStyle.h"

class PageSats {
public:
    PageSats(GPS& gps);
    ~PageSats();
private:
    void update();
    static void task(lv_task_t* task);
    GPS&     _gps;
    LVPage*  _page;
    LVChart* _chart;
    lv_chart_series_t* _tracked_max_series;
    lv_chart_series_t* _tracked_min_series;
    LVStyle  _container_style;
    LVStyle  _chart_style;

    uint16_t _tracked_max = 0;
    uint16_t _tracked_min = 0;

    time_t _last_time = 0;
};

#endif // _PAGE_SATS_H_

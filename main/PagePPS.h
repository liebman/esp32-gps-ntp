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

#ifndef _PAGE_PPS_H_
#define _PAGE_PPS_H_

#include "PPS.h"
#include "LVPage.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PagePPS {
public:
    PagePPS(PPS& gps_pps, PPS& rtc_pps);
    ~PagePPS();

    PagePPS(PagePPS&) = delete;
    PagePPS& operator=(PagePPS&) = delete;

private:
    void update();
    static void task(lv_task_t* task);
    PPS&     _gps_pps;
    PPS&     _rtc_pps;

    LVPage*  _page;
    LVLabel* _gps_time;
    LVLabel* _rtc_time;
    LVLabel* _gps_interval;
    LVLabel* _rtc_interval;
    LVLabel* _gps_minmax;
    LVLabel* _gps_shortlong;
    LVLabel* _rtc_minmax;
    LVLabel* _rtc_shortlong;
    LVLabel* _rtc_offset;
    LVStyle  _container_style;
};

#endif // _PAGE_GPS_H_

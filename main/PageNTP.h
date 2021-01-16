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

#ifndef _PAGE_NTP_H_
#define _PAGE_NTP_H_

#include "NTP.h"
#include "SyncManager.h"
#include "LVPage.h"
#include "LVTable.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PageNTP {
public:
    PageNTP(NTP& ntp, SyncManager& syncman);
    ~PageNTP();

    PageNTP(PageNTP&) = delete;
    PageNTP& operator=(PageNTP&) = delete;

private:
    void update();
    static void task(lv_task_t* task);
    NTP&         _ntp;
    SyncManager& _syncman;
    LVPage*      _page;
    LVLabel*     _datetime;
    LVTable*     _table;
    LVStyle      _container_style;
    char         _time_buf[32] = {0};
};

#endif // _PAGE_NTP_H_

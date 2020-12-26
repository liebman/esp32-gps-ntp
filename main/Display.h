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

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "LVTabView.h"

class Display
{
public:
    static Display& getDisplay();
    ~Display();
    bool begin();
    LVPage* newPage(const char* name);
    bool lock(int timeout_ms); // negitive timeout is wait forever
    void unlock();
    uint32_t getIdleTime();
private:
    Display();
    SemaphoreHandle_t _lock;
    LVTabView*        _tabview;
    lv_disp_t*        _disp = nullptr;
    static void task(void* data);
    static void tickAction(void* data);
};
#endif // _DISPLAY_H

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

#include "WithDisplayLock.h"
#include "Display.h"
#include "esp_log.h"

static const char* TAG = "WithDisplayLock";

WithDisplayLock::WithDisplayLock(const std::function<void()>& got_lock, const std::function<void()>& got_no_lock)
{
    _locked = Display::getDisplay().lock(1000);
    if (_locked)
    {
        ESP_LOGD(TAG, "calling got_lock exists=%d", got_lock != nullptr);
        got_lock();
    }
    else if (got_no_lock)
    {
        ESP_LOGD(TAG, "got_no_lock!");
        got_no_lock();
    }
}

WithDisplayLock::WithDisplayLock()
{
    while(true)
    {
        _locked = Display::getDisplay().lock(1000);
        if (_locked)
        {
            return;
        }
        ESP_LOGE(TAG, "failed to get lock, will retry!");
    }
}

WithDisplayLock::~WithDisplayLock()
{
    if (_locked)
    {
        ESP_LOGD(TAG, "unlocking!");
        Display::getDisplay().unlock();
    }
}

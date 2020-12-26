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

#include "MicroSecondTimer.h"
#include "esp_log.h"

static const char* TAG = "TIMER";

MicroSecondTimer::MicroSecondTimer()
{
    ESP_LOGI(TAG, "::begin configuring and starting microsecond timer");
    timer_config_t tc = {
        .alarm_en    = TIMER_ALARM_DIS,
        .counter_en  = TIMER_PAUSE,
        .intr_type   = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_DIS,
        .divider     = 80,  // microseconds second
    };
    esp_err_t err = timer_init(MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM, &tc);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "timer_init failed: group=%d timer=%d err=%d (%s)",
                 MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM, err, esp_err_to_name(err));
    }
    timer_set_counter_value(MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM, 0x00000000ULL);
    timer_start(MICRO_SECOND_TIMER_GROUP_NUM, MICRO_SECOND_TIMER_NUM);
}

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

#include "PPS.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include <string.h>
#include <sys/time.h>
#include "soc/soc.h"
#include "driver/timer.h"

static const char* TAG = "PPS";

PPS::PPS(MicroSecondTimer& timer, pps_data_t* data, PPS* ref)
: _timer(timer),
  _data(data),
  _ref(ref)
{
    memset(data, 0, sizeof(pps_data_t));
    ESP_LOGI(TAG, "pps data %u bytes @ 0x%08x", sizeof(pps_data_t), (uint32_t)data);
    if (ref != nullptr)
    {
        data->pps_ref = ref->_data;
    }
}

bool PPS::begin(gpio_num_t pps_pin, bool expect_negedge)
{
    _pin = pps_pin;
    _data->pps_pin = pps_pin;

    if (_pin != GPIO_NUM_NC)
    {
        ESP_LOGI(TAG, "::begin configuring PPS pin %d", _pin);
        gpio_set_direction(_pin, GPIO_MODE_INPUT);

        ESP_LOGI(TAG, "::begin setup ppsISR for highint5");
        ESP_INTR_DISABLE(31);
        intr_matrix_set(1, ETS_GPIO_INTR_SOURCE, 31);
        ESP_INTR_ENABLE(31);
        gpio_set_intr_type(_pin, expect_negedge ? GPIO_INTR_NEGEDGE : GPIO_INTR_POSEDGE);
        gpio_intr_enable(_pin);
    }

    return true;
}

int PPS::getLevel()
{
    return gpio_get_level(_pin);
}

/**
 * get current time & microseconds.
*/
time_t PPS::getTime(struct timeval* tv)
{
    if (tv == nullptr)
    {
        return _data->pps_time;
    }

    // edge case!  both seconds and _last_timer only change once a second, however,
    // it changes via an interrupt so we make sure we have good values by making sure
    // it is the same on second look
    do
    {
        tv->tv_sec  = _data->pps_time;
        tv->tv_usec = _timer.getValue() - _data->pps_last;
        // timer could be slightly off, insure microseconds is not returned as (or more) than a full second!
        if (tv->tv_usec > 999999)
        {
            tv->tv_usec = 999999;
        }
    } while (tv->tv_sec != _data->pps_time); // insure we stay on the same seconds (to go with the microseconds)
    return tv->tv_sec;
}

/**
 * set the time, seconds only
*/
void PPS::setTime(time_t time)
{
    _data->pps_time = time;
}

/**
 * get the minimum time in microseconds between PPS pulses
*/
uint32_t PPS::getTimerMin()
{
    return _data->pps_min;
}

/**
 * get the last timer value
*/
uint32_t PPS::getTimerLast()
{
    return _data->pps_last;
}

/**
 * get the maximum time in microseconds between PPS pulses
*/
uint32_t PPS::getTimerMax()
{
    return _data->pps_max;
}

/**
 * get the interval in microseconds between PPS pulses
*/
uint32_t PPS::getTimerInterval()
{
    return _data->pps_interval;
}

/**
 * get the number of PPS pulses considered short (and invalid)
*/
uint32_t PPS::getTimerShort()
{
    return _data->pps_short;
}

/**
 * get the number of PPS pulses considered long (and invalid)
*/
uint32_t PPS::getTimerLong()
{
    return _data->pps_long;
}

/**
 * get the offset from ref
*/
int32_t PPS::getOffset()
{
    return _data->pps_offset;
}

/**
 * reset the offset from ref
*/
void PPS::resetOffset()
{
    _data->pps_offset = 0;
}

/**
 * set disabled preventing the ISR from counting
*/
void PPS::setDisable(bool disable)
{
    _data->pps_disabled = disable;
}

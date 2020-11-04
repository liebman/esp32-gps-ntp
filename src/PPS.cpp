#include "PPS.h"
#include "LatencyPin.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include <sys/time.h>
#include <soc/soc.h>
#include "ulp_main.h"

static const char* TAG = "PPS";


#if defined(CONFIG_GPSNTP_SHORT_TIME)
#define PPS_SHORT_VALUE  CONFIG_GPSNTP_SHORT_TIME
#else
#define PPS_SHORT_VALUE  999500
#endif

#if defined(CONFIG_GPSNTP_MISS_TIME)
#define PPS_MISS_VALUE CONFIG_GPSNTP_MISS_TIME
#else
#define PPS_MISS_VALUE  1000500 // 500 usec max
#endif


PPS::PPS()
{
}

bool PPS::begin(gpio_num_t pps_pin, bool expect_negedge)
{
    static bool isr_init = false;
    _pin = pps_pin;
    if (expect_negedge)
    {
        _expect_skip = 1;
    }
    else
    {
        _expect_skip = 0;
    }

    if (_pin != GPIO_NUM_NC)
    {
        if (!isr_init)
        {
            isr_init = true;
            ESP_LOGI(TAG, "::begin installing gpio isr service!");
            esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM|ESP_INTR_FLAG_LEVEL3);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "::begin failed to install gpio isr service: %d (%s)", err, esp_err_to_name(err));
                return false;
            }
        }
        ESP_LOGI(TAG, "::begin configuring PPS pin %d", _pin);
        gpio_set_direction(_pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(_pin, GPIO_PULLUP_ONLY);
        //hook isr handler for specific gpio pin
        gpio_isr_handler_add(_pin, pps, this);
        ESP_LOGI(TAG, "::begin setup ppsISR");
        gpio_set_intr_type(_pin, GPIO_INTR_HIGH_LEVEL);
        gpio_intr_enable(_pin);
    }

    return true;
}


static gpio_int_type_t next_level[2] {GPIO_INTR_HIGH_LEVEL,GPIO_INTR_LOW_LEVEL};

void IRAM_ATTR PPS::pps(void* data)
{
    ulp_latency_flag = 1;
    PPS* pps = (PPS*)data;
    uint64_t current = esp_timer_get_time();
#ifdef PPS_LATENCY_OUTPUT
    gpio_set_level(LATENCY_PIN, 1);
#endif
#if 0
    // print warning if level is not what we expect!
    int level = gpio_get_level(pps->_pin);
    if (pps->_expect != level)
    {
        ets_printf("ERROR: %d expect (%d) != level (%d)\n", pps->_pin, pps->_expect, level);
    }
#endif
    gpio_set_intr_type(pps->_pin, next_level[pps->_expect]);

    uint32_t interval = current - pps->_last_timer;

    if (pps->_expect == pps->_expect_skip)
    {
        pps->_expect ^= 1;
        ulp_latency_flag = 0;
#ifdef PPS_LATENCY_OUTPUT
        gpio_set_level(LATENCY_PIN, 0);
#endif
        return;
    }
    pps->_expect ^= 1;

    pps->_last_timer = current;

    // could be the first time, lets skip the statistics and time keeping in that case
    if (pps->_last_timer == 0)
    {
        ulp_latency_flag = 0;
#ifdef PPS_LATENCY_OUTPUT
        gpio_set_level(LATENCY_PIN, 0);
#endif
        return;
    }

    // increment the time
    pps->_time += 1;

    // no stats for the first few seconds
    if (pps->_time < 3)
    {
        ulp_latency_flag = 0;
#ifdef PPS_LATENCY_OUTPUT
        gpio_set_level(LATENCY_PIN, 0);
#endif
        return;
    }

    // detect min/max statistics
    if (pps->_timer_min == 0 || interval < pps->_timer_min)
    {
        pps->_timer_min = interval;
    }

    if (pps->_timer_max == 0 || interval > pps->_timer_max)
    {
        pps->_timer_max = interval;
    }

    if (interval < PPS_SHORT_VALUE)
    {
        pps->_timer_short += 1;
        pps->_timer_min = 0;
    }
    else if (interval > PPS_MISS_VALUE)
    {
        pps->_timer_long += 1;
        pps->_timer_max = 0;
    }
    ulp_latency_flag = 0;
#ifdef PPS_LATENCY_OUTPUT
    gpio_set_level(LATENCY_PIN, 0);
#endif
}

/**
 * get current time & microseconds.
*/
void PPS::getTime(struct timeval* tv)
{
    // edge case!  both seconds and _last_timer only change once a second, however,
    // it changes via an interrupt so we make sure we have good values by making sure
    // it is the same on second look
    do
    {
        tv->tv_sec  = _time;
        tv->tv_usec = esp_timer_get_time() - _last_timer;
        // timer could be slightly off, insure microseconds is not returned as a full second!
        if (tv->tv_usec > 999999)
        {
            tv->tv_usec = 999999;
        }
    } while (tv->tv_sec != _time); // insure we stay on the same seconds (to go with the microseconds)
}

/**
 * set the time, seconds only
*/
void PPS::setTime(time_t time)
{
    _time = time;
}

/**
 * get the minimum time in microseconds between PPS pulses
*/
uint32_t PPS::getTimerMin()
{
    return _timer_min;
}

/**
 * get the maximum time in microseconds between PPS pulses
*/
uint32_t PPS::getTimerMax()
{
    return _timer_max;
}

/**
 * get the number of PPS pulses considered short (and invalid)
*/
uint32_t PPS::getTimerShort()
{
    return _timer_short;
}

/**
 * get the number of PPS pulses considered long (and invalid)
*/
uint32_t PPS::getTimerLong()
{
    return _timer_long;
}

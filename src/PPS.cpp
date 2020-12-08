#include "PPS.h"
#include "LatencyPin.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include <sys/time.h>
#include "soc/soc.h"
#include "driver/timer.h"

static const char* TAG = "PPS";

//#define USE_LEVEL_INTERRUPT
//#define USE_INTERRUPT_SERVICE

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


PPS::PPS(MicroSecondTimer& timer, pps_data_t* data, PPS* ref)
: _timer(timer),
  _data(data),
  _ref(ref)
{
}

bool PPS::begin(gpio_num_t pps_pin, bool expect_negedge)
{
#ifdef USE_INTERRUPT_SERVICE
    static bool isr_init = false;
#endif
    _pin = pps_pin;
    _data->pps_pin = pps_pin;

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
#ifdef USE_INTERRUPT_SERVICE
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
#endif
        ESP_LOGI(TAG, "::begin configuring PPS pin %d", _pin);
        gpio_set_direction(_pin, GPIO_MODE_INPUT);
        // TODO: fix this!
        ESP_LOGW(TAG, "::begin TODO: fix this: 'if (_pin == 27)' !!!");
        if (_pin == 27)
        {
            gpio_set_pull_mode(_pin, GPIO_PULLUP_ONLY);
        }
#ifdef USE_INTERRUPT_SERVICE
        //hook isr handler for specific gpio pin
        gpio_isr_handler_add(_pin, pps, this);
#else
        ESP_INTR_DISABLE(31);
        intr_matrix_set(1, ETS_GPIO_INTR_SOURCE, 31);
        ESP_INTR_ENABLE(31);
#endif

        ESP_LOGI(TAG, "::begin setup ppsISR");
#ifdef USE_LEVEL_INTERRUPT
        gpio_set_intr_type(_pin, GPIO_INTR_HIGH_LEVEL);
#else
        gpio_set_intr_type(_pin, expect_negedge ? GPIO_INTR_NEGEDGE : GPIO_INTR_POSEDGE);
#endif
        gpio_intr_enable(_pin);
    }

    return true;
}

int PPS::getLevel()
{
    return gpio_get_level(_pin);
}

#if USE_INTERRUPT_SERVICE
#ifdef USE_LEVEL_INTERRUPT
static gpio_int_type_t next_level[2] {GPIO_INTR_HIGH_LEVEL,GPIO_INTR_LOW_LEVEL};
#endif

#ifdef LATENCY_OUTPUT
#define LATENCY_START() GPIO.out_w1ts = 1 << LATENCY_PIN
#define LATENCY_END()   GPIO.out_w1tc = 1 << LATENCY_PIN
#else
#define LATENCY_START()
#define LATENCY_END()
#endif

void IRAM_ATTR PPS::pps(void* data)
{
    PPS* pps = (PPS*)data;
    LATENCY_START();

    uint32_t current = esp_timer_get_time();

#ifdef USE_LEVEL_INTERRUPT
#if 0
    // print warning if level is not what we expect!
    int level = gpio_get_level(pps->_pin);
    if (pps->_expect != level)
    {
        ets_printf("ERROR: %d expect (%d) != level (%d)\n", pps->_pin, pps->_expect, level);
    }
#endif

    GPIO.pin[pps->_pin].int_type = next_level[pps->_expect];

    if (pps->_expect == pps->_expect_skip)
    {
        pps->_expect ^= 1;
        LATENCY_END();
        return;
    }
    pps->_expect ^= 1;
#endif
    uint64_t last = pps->_data->pps_last;
    pps->_data->pps_last = current;
    if (pps->_disabled)
    {
        LATENCY_END();
        return;
    }

    // it's the first time, lets skip the statistics and time keeping in that case
    if (last == 0)
    {
        LATENCY_END();
        return;
    }

    // increment the time
    pps->_data->pps_time += 1;

    int32_t last_offset = 0;

    if (pps->_ref != nullptr)
    {
        last_offset = pps->_offset;

        int32_t delta = pps->_data->pps_last - pps->_ref->_data->pps_last;
        // we only care about microseconds so we will keep the delta
        // between -500000 and 500000.  Yea, if the reference stops then this is invalid
        if (delta > 500000)
        {
            delta -= 1000000;
        }
        else if (delta < -500000)
        {
            delta += 1000000;
        }
        pps->_offset = delta;
    }

    // no stats for the first few seconds
    if (pps->_data->pps_time < 3)
    {
        LATENCY_END();
        return;
    }

    uint32_t interval = current - last;
    if (interval < PPS_SHORT_VALUE)
    {
#if 1
        ets_printf("ERROR: %d: short (%d) @%lu lo=%d\n", pps->_pin, interval, pps->_data->pps_time, last_offset);
#endif
        pps->_timer_short += 1;
        pps->_timer_min = 0;
        LATENCY_END();
        return;
    }

    if (pps->_timer_min == 0 || interval < pps->_timer_min)
    {
        pps->_timer_min = interval;
    }

    if (interval > PPS_MISS_VALUE)
    {
#if 1
        ets_printf("ERROR: %d: long (%d) @%lu lo=%d\n", pps->_pin, interval, pps->_data->pps_time, last_offset);
#endif
        pps->_timer_long += 1;
        pps->_timer_max = 0;
        LATENCY_END();
        return;
    }

    if (pps->_timer_max == 0 || interval > pps->_timer_max)
    {
        pps->_timer_max = interval;
    }
    LATENCY_END();
}
#endif

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
    return _timer_short;
}

/**
 * get the number of PPS pulses considered long (and invalid)
*/
uint32_t PPS::getTimerLong()
{
    return _timer_long;
}

/**
 * get the offset from ref
*/
int32_t PPS::getOffset()
{
    return _offset;
}

/**
 * reset the offset from ref
*/
void PPS::resetOffset()
{
    _offset = 0;
}

/**
 * set disabled preventing the ISR from counting
*/
void PPS::setDisable(bool disable)
{
    _disabled = disable;
}

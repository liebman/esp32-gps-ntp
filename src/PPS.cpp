#include "PPS.h"
#include "string.h"
#include "minmea.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include <sys/time.h>
#include <soc/soc.h>

static const char* TAG = "PPS";


#ifndef PPS_TASK_PRI
#define PPS_TASK_PRI configMAX_PRIORITIES-1
#endif

#ifndef PPS_TASK_CORE
#define PPS_TASK_CORE 1
#endif

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


PPS::PPS(MicroSecondTimer& timer, gpio_num_t pps_pin, bool expect_negedge)
: _timer(timer),
  _pin(pps_pin)
{
    if (expect_negedge)
    {
        _expect_skip = 1;
    }
    else
    {
        _expect_skip = 0;
    }
}

bool PPS::begin()
{
    static bool isr_init = false;

    esp_err_t err = ESP_OK;

    if (_pin != GPIO_NUM_NC)
    {
#ifdef PPS_LATENCY_PIN
        ESP_LOGI(TAG, "::begin configuring PPS_LATENCY_PIN pin %d", PPS_LATENCY_PIN);
        gpio_config_t io_conf;
        io_conf.pin_bit_mask = PPS_LATENCY_SEL;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        err = gpio_config(&io_conf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "failed to init PPS_LATENCY_PIN pin as output: %d '%s'", err, esp_err_to_name(err));
        }
#endif
        if (!isr_init)
        {
            isr_init = true;
            ESP_LOGI(TAG, "::begin installing gpio isr service!");
            esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM|ESP_INTR_FLAG_LEVEL2);
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

#if 0
    ESP_LOGI(TAG, "::begin create PPS task at priority %d core %d", PPS_TASK_PRI, PPS_TASK_CORE);
    xTaskCreatePinnedToCore(ppsTask, "PPS", 4096, this, PPS_TASK_PRI, &_task, PPS_TASK_CORE);
#endif

    return true;
}


uint32_t PPS::getCount()
{
    return _count;
}

time_t PPS::getTime(uint32_t* microseconds)
{
    uint32_t count;
    // edge case!  both seconds and _last_timer only change once a second, however,
    // it changes via an interrupt so we make sure we have good values by making sure
    // it is the same on second look
    do
    {
        count = _count;
        if (microseconds != nullptr)
        {
            *microseconds = _timer.getValue() - _last_timer;
            // timer could be slightly off, insure microseconds is not returned as a full second!
            if (*microseconds > 999999)
            {
                *microseconds = 999999;
            }
        }
    } while (count != _count); // insure we stay on the same seconds (to go with the microseconds)

    return (time_t)_count;
}

void PPS::setTime(time_t time)
{
    _count = (uint32_t)time;
}

uint64_t PPS::getLastTimer()
{
    uint64_t last = _last_timer;
    if (last != _last_timer)
    {
        last = _last_timer;
    }
    return last;
}

void PPS::resetMicroseconds()
{
    // TODO: do I need a lock now?
    _last_timer = _timer.getValue();
}

uint32_t PPS::getTimerMax()
{
    return _timer_max;
}

uint32_t PPS::getTimerMin()
{
    return _timer_min;
}

uint32_t PPS::getMissed()
{
    return _missed;
}

uint32_t PPS::getShort()
{
    return _short;
}

uint32_t PPS::getShortLast()
{
    return _short_last;
}

uint32_t PPS::getLast()
{
    return _last;
}

uint32_t PPS::getHighTime()
{
    return _high_time;
}

uint32_t PPS::getLowTime()
{
    return _low_time;
}

#if 0
void PPS::ppsTask(void* data)
{
    GPS* gps = (GPS*)data;
    uint64_t value;

    ESP_LOGI(TAG, "::ppsTask - starting!");

    while (true)
    {
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000)))
        {
            if (pps->_set_time && pps->_set_time_func)
            {
                pps->_set_time_func(pps->_time);
                pps->_set_time = false;
            }
            esp_err_t err = timer_get_counter_value(GPS_TIMER_GROUP, GPS_TIMER_NUM, &value);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "::ppsTask timer_get_counter_value failed: group=%d timer=%d err=%d (%s)", GPS_TIMER_GROUP, GPS_TIMER_NUM, err, esp_err_to_name(err));
                continue;
            }
            ESP_LOGV(TAG, "ppsTask: notify delay %u us", (uint32_t)value);
        }
    }
}
#endif

static gpio_int_type_t next_level[2] {GPIO_INTR_HIGH_LEVEL,GPIO_INTR_LOW_LEVEL};

void IRAM_ATTR PPS::pps(void* data)
{
#ifdef PPS_LATENCY_PIN
    gpio_set_level(PPS_LATENCY_PIN, 1);
#endif
    PPS* pps = (PPS*)data;
    uint64_t current = pps->_timer.getValue();
    int level = gpio_get_level(pps->_pin);
    if (pps->_expect != level)
    {
        ets_printf("ERROR: %d expect (%d) != level (%d)\n", pps->_pin, pps->_expect, level);
    }

    gpio_set_intr_type(pps->_pin, next_level[pps->_expect]);

    uint32_t interval = current - pps->_last_timer;

    if (pps->_expect == pps->_expect_skip)
    {
        pps->_high_time = interval;
        pps->_expect ^= 1;
#ifdef PPS_LATENCY_PIN
        gpio_set_level(PPS_LATENCY_PIN, 0);
#endif
        return;
    }
    pps->_expect ^= 1;

    if (pps->_last_timer == 0)
    {
        pps->_last_timer = current;
#ifdef PPS_LATENCY_PIN
        gpio_set_level(PPS_LATENCY_PIN, 0);
#endif
        return;
    }

    pps->_low_time = interval;

    pps->_last = interval;
    pps->_count += 1;


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
        pps->_short += 1;
        pps->_short_last = interval;
        pps->_timer_min = 0;
    }
    else if (interval > PPS_MISS_VALUE)
    {
        pps->_missed += 1;
        pps->_miss_last = interval;
        pps->_timer_max = 0;
    }

    pps->_last_timer = current;
#if 0
    BaseType_t hpwake;
    vTaskNotifyGiveFromISR(pps->_task, &hpwake);
    if (hpwake)
    {
        portYIELD_FROM_ISR();
    }
#endif

#ifdef PPS_LATENCY_PIN
    gpio_set_level(PPS_LATENCY_PIN, 0);
#endif
}

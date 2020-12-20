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

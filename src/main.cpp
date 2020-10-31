#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "Display.h"
#include "LatencyPin.h"
#include "PPS.h"
#include "GPS.h"

#include "PageAbout.h"
#include "PagePPS.h"
#include "PageGPS.h"
#include "PageSats.h"

#ifdef __cplusplus
//
// app_main() is called from C
//
extern "C" {
    void app_main(void);
}
#endif

#if defined(CONFIG_GPSNTP_RTC_DRIFT_MAX)
#define RTC_DRIFT_MAX CONFIG_GPSNTP_RTC_DRIFT_MAX
#else
#define RTC_DRIFT_MAX 500
#endif

#define GPS_RX_PIN (GPIO_NUM_33)
#define GPS_TX_PIN (GPIO_NUM_32)
#define GPS_PPS_PIN (GPIO_NUM_26)
#define RTC_PPS_PIN (GPIO_NUM_27)
#define SDA_PIN (GPIO_NUM_16)
#define SCL_PIN (GPIO_NUM_17)
#define TCH_IRQ_PIN (GPIO_NUM_36)
#define TCH_IRQ_SEL (GPIO_SEL_36)
#define TFT_LED_PIN (GPIO_NUM_4)
#define TFT_LED_SEL (GPIO_SEL_4)

static const char* TAG = "main";
static PPS gps_pps;
static GPS gps;
static void init(void* data)
{
    (void)data;
    ESP_LOGI(TAG, "init: with priority %d core %d", uxTaskPriorityGet(nullptr), xPortGetCoreID());

    esp_err_t err;
    gpio_config_t io_conf;

#ifdef LATENCY_OUTPUT
    ESP_LOGI(TAG, "configuring LATENCY_PIN pin %d", LATENCY_PIN);
    io_conf.pin_bit_mask = LATENCY_SEL;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to init LATENCY_PIN pin as output: %d '%s'", err, esp_err_to_name(err));
    }
#endif

    ESP_LOGI(TAG, "init: starting display");
    if (!Display::getDisplay().begin())
    {
        ESP_LOGE(TAG, "init: failed to initialize display!");
    }

    // turn the backlight on.
    ESP_LOGI(TAG,  "turning backlight on");
    io_conf.pin_bit_mask = TFT_LED_SEL;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to init backlight pin as output: %d '%s'", err, esp_err_to_name(err));
    }
    // turn the backlight on.
    gpio_set_level(TFT_LED_PIN, 1);

    new PagePPS(gps_pps);
    new PageGPS(gps);
    new PageSats(gps);
    new PageAbout();

    // start gps watching NMEA messages
    // swap rx/tx as GPS_RX is our TX!
    if (!gps.begin(GPS_RX_PIN, GPS_TX_PIN))
    {
        ESP_LOGE(TAG, "failed to start gps!");
    }

    // start pps watching gps
    if (!gps_pps.begin(GPS_PPS_PIN))
    {
        ESP_LOGE(TAG, "failed to start pps!");
    }

    vTaskDelete(NULL);
}

void app_main()
{
    ESP_LOGI(TAG, "Starting with priority %d core %d", uxTaskPriorityGet(nullptr), xPortGetCoreID());



    xTaskCreatePinnedToCore(&init, "init", 4096, nullptr, 1, nullptr, 1);

    bool was_valid = false;
    while(true)
    {
        bool now_valid = gps.getValid();
        if (was_valid != now_valid)
        {
            if (now_valid)
            {
                gps_pps.setTime(gps.getRMCTime());
                struct timeval tv;
                uint32_t microseconds;
                tv.tv_sec = gps_pps.getTime(&microseconds);
                tv.tv_usec = microseconds;
                settimeofday(&tv, nullptr);
                ESP_LOGW(TAG, "gps gained validity, time has been set!");
            }
            else
            {
                ESP_LOGW(TAG, "gps lost validity!");
            }
            was_valid = now_valid;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

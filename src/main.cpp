#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "Config.h"
#include "Display.h"
#include "Network.h"
#include "DS3231.h"
#include "MicroSecondTimer.h"
#include "PPS.h"
#include "GPS.h"
#include "NTP.h"
#include "SyncManager.h"

#include "PageAbout.h"
#include "PageConfig.h"
#include "PagePPS.h"
#include "PageDelta.h"
#include "PageSync.h"
#include "PageGPS.h"
#include "PageSats.h"
#include "PageTask.h"
#include "PageNTP.h"

#define LATENCY_PIN 2
#define LATENCY_SEL (1<<LATENCY_PIN)

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
#ifdef NEW_BOARD
#define GPS_PPS_PIN (GPIO_NUM_35)
#define RTC_PPS_PIN (GPIO_NUM_26)
#else
#define GPS_PPS_PIN (GPIO_NUM_26)
#define RTC_PPS_PIN (GPIO_NUM_27)
#endif
#define SDA_PIN (GPIO_NUM_16)
#define SCL_PIN (GPIO_NUM_17)
#define TFT_LED_PIN (GPIO_NUM_4)
#define TFT_LED_SEL (GPIO_SEL_4)
#define TCH_IRQ_PIN ((gpio_num_t)CONFIG_LV_TOUCH_PIN_IRQ)
#if CONFIG_LV_TOUCH_PIN_IRQ < 32
#define TCH_IRQ_SEL (1<<CONFIG_LV_TOUCH_PIN_IRQ)
#else
#define TCH_IRQ_SEL ((uint64_t)(((uint64_t)1)<<CONFIG_LV_TOUCH_PIN_IRQ))
#endif

static const char* TAG = "main";


extern pps_data_t rtc_pps_data; // in highint5.S
extern pps_data_t gps_pps_data; // in highint5.S

//#define NO_GPS_PPS
//#define NO_RTC_PPS

static Config config;
static MicroSecondTimer usec_timer;
static PPS gps_pps(usec_timer, &gps_pps_data);
static PPS rtc_pps(usec_timer, &rtc_pps_data, &gps_pps); // use gps_pps as ref.
static GPS gps;
static DS3231 rtc;
static NTP ntp(rtc_pps);
static SyncManager syncman(gps, rtc, gps_pps, rtc_pps);

static void apply_config()
{
    syncman.setBias(config.getBias());
    syncman.setTarget(config.getTarget());
}

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

    // initialize I2C_NUM_0 for the MCP23017T
    err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_driver_install: I2C_NUM_%d %d (%s)", I2C_NUM_0, err, esp_err_to_name(err));
    }
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDA_PIN;
    conf.scl_io_num = SCL_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = 400000;
    err = i2c_param_config(I2C_NUM_0, &conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_param_config: I2C_NUM_%d %d (%s)", I2C_NUM_0, err, esp_err_to_name(err));
    }

    while (!rtc.begin())
    {
        ESP_LOGE(TAG, "failed to start rtc!");
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // start gps watching NMEA messages
    // swap rx/tx as GPS_RX is our TX!
    if (!gps.begin(GPS_RX_PIN, GPS_TX_PIN))
    {
        ESP_LOGE(TAG, "failed to start gps!");
    }

#ifndef NO_GPS_PPS
    // start pps watching gps
    if (!gps_pps.begin(GPS_PPS_PIN))
    {
        ESP_LOGE(TAG, "failed to start GPS pps!");
    }
#endif

#ifndef NO_RTC_PPS
    // start pps watching rtc
    if (!rtc_pps.begin(RTC_PPS_PIN, true))
    {
        ESP_LOGE(TAG, "failed to start RTC pps!");
    }

    // wait for the RTC PPS signal to be low, the first half of a secondm, so we
    // don't do this on a second boundry as the RTC PPS could get or miss an increment.
    ESP_LOGI(TAG, " waiting a change in seconds");
    time_t now = rtc_pps.getTime(nullptr);
    while(rtc_pps.getTime(nullptr) == now)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG, "setting time on RTC pps");
    struct tm tm;
    rtc.getTime(&tm);
    rtc_pps.setTime(mktime(&tm));
#endif

    // start NTP services
    ntp.begin();

    // start the sync manager
    syncman.begin();

    new PagePPS(gps_pps, rtc_pps);
    new PageNTP(ntp, syncman);
    new PageSync(syncman);
    new PageDelta(syncman);
    new PageGPS(gps);
    new PageSats(gps);
    new PageTask();
    new PageConfig(config, apply_config);
    new PageAbout();

    vTaskDelete(NULL);
}

void app_main()
{
    ESP_LOGI(TAG, "Starting with priority %d core %d", uxTaskPriorityGet(nullptr), xPortGetCoreID());

    //Initialize NVS
    ESP_LOGI(TAG, "initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    config.begin();
    config.load();
    apply_config();

    xTaskCreatePinnedToCore(&init, "init", 4096, nullptr, 1, nullptr, 1);

    ESP_LOGI(TAG, "initializing Network");
    Network::getNetwork().begin(config.getWiFiSSID(), config.getWiFiPassword());

#if 0
    esp_err_t err;

    // lets monitor the touch input.
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = TCH_IRQ_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to init TOUCH IRQ pin as input: %d '%s'", err, esp_err_to_name(err));
    }
#endif

    bool was_valid = false;
    bool is_on = true;
    time_t last_touch = time(nullptr);
    time_t last_time = last_touch;

    while(true)
    {
        struct timeval gps_tv;
        gps_pps.getTime(&gps_tv);
        time_t gps_time = gps.getRMCTime();
        bool now_valid = gps.getValid();
        if (was_valid != now_valid
        || (now_valid 
            && gps_tv.tv_usec > 800000 
            && gps_tv.tv_usec < 900000 
            && gps_time != gps_tv.tv_sec))
        {
            if (now_valid)
            {
                gps_pps.setTime(gps.getRMCTime());
                struct timeval tv;
                gps_pps.getTime(&tv);
                settimeofday(&tv, nullptr);
                ESP_LOGW(TAG, "gps gained validity, time has been set!");
            }
            else
            {
                ESP_LOGW(TAG, "gps lost validity!");
            }
            was_valid = now_valid;
        }

        time_t now = time(nullptr);
#if 1
        static time_t last_report = 0;
        if (now > last_report+60)
        {
            ESP_LOGI(TAG, "now valid: %d is_on: %d idle: %u (%u)", now_valid, is_on, (uint32_t)now-(uint32_t)last_touch, (uint32_t)Display::getDisplay().getIdleTime());
            last_report = now;
        }
#endif
        // detect time jump when time is set.
        if (now - last_time > 300)
        {
            ESP_LOGI(TAG, "time jump %u seconds, reseting last_touch", (uint32_t)now-(uint32_t)last_time);
            last_touch = now;
        }
        last_time = now;

        if (gpio_get_level(TCH_IRQ_PIN) == 0 && gpio_get_level(TCH_IRQ_PIN) == 0)
        {
            //ESP_LOGI(TAG, "tch_irq: %d", gpio_get_level(TCH_IRQ_PIN));
            last_touch = now;
        }

        if ((now - last_touch) > 3600) // 1 hr
        {
            if (is_on)
            {
                ESP_LOGI(TAG, "turning OFF backlight");
                gpio_set_level(TFT_LED_PIN, 0);
                is_on = false;
            }
        }
        else if (!is_on)
        {
            ESP_LOGI(TAG, "turning ON backlight");
            gpio_set_level(TFT_LED_PIN, 1);
            is_on = true;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

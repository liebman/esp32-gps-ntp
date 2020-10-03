#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "esp_log.h"

#include "Display.h"
#include "GPS.h"
#include "PageGPS.h"
#include "PageSats.h"
#include "PageAbout.h"

#ifdef __cplusplus
//
// app_main() is called from C
//
extern "C" {
    void app_main(void);
}
#endif

#define GPS_RX_PIN (GPIO_NUM_33)
#define GPS_TX_PIN (GPIO_NUM_32)
#define GPS_PPS_PIN (GPIO_NUM_26)
#define SDA_PIN (GPIO_NUM_16)
#define SCL_PIN (GPIO_NUM_17)
#define TCH_IRQ_PIN (GPIO_NUM_36)
#define TCH_IRQ_SEL (GPIO_SEL_36)
#define TFT_LED_PIN (GPIO_NUM_4)
#define TFT_LED_SEL (GPIO_SEL_4)

static const char* TAG = "app_main";
static GPS gps(GPS_PPS_PIN);

void app_main() 
{
    ESP_LOGI(TAG, "Starting with priority %d", uxTaskPriorityGet(nullptr));

    // initialize I2C_NUM_0 for the MCP23017T
    esp_err_t err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
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

    // initialize the display
    ESP_LOGI(TAG, "initializing the Display");
    if (!Display::getDisplay().begin())
    {
        ESP_LOGE(TAG, "failed to initialize the Display!");
    }

    new PageGPS(gps);
    new PageSats(gps);
    new PageAbout();

    // turn the backlight on.
    ESP_LOGI(TAG,  "turning backlight on");

    gpio_config_t io_conf;
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

    // swap rx/tx as GPS_RX is our TX!
    if (!gps.begin(GPS_RX_PIN, GPS_TX_PIN))
    {
        ESP_LOGE(TAG, "failed to start gps!");
    }

    // lets monitor the touch input.
    io_conf.pin_bit_mask = TCH_IRQ_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    err = gpio_config(&io_conf);

    time_t last_touch = time(nullptr);
    bool is_on = true;
    while(true)
    {
        time_t now = time(nullptr);
        if (gpio_get_level(TCH_IRQ_PIN) == 0)
        {
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

        vTaskDelay(10);
    }
}

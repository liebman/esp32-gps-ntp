#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "esp_log.h"

#include "TeaMakerPins.h"
#include "Display.h"
#include "MCP23017.h"
#include "PageAbout.h"
#include "GPS.h"

#ifdef __cplusplus
//
// app_main() is called from C
//
extern "C" {
    void app_main(void);
}
#endif

#define GPS_RX_PIN (GPIO_NUM_4)
#define GPS_TX_PIN (GPIO_NUM_2)

static const char* TAG = "app_main";
static MCP23017 mcp23017;
static GPS gps;

void app_main() 
{
    ESP_LOGI(TAG, "Starting!");

    // configure reset gpio as output, no interrupts or pull-up/down
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = MCP23017_RST_SEL;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to init MCP23017 reset pin as output: %d '%s'", err, esp_err_to_name(err));
    }
    // turn the backlight on.
    ESP_LOGI(TAG,  "removing reset from MCP23017");
    gpio_set_level(MCP23017_RST_PIN, 1);

    // initialize I2C_NUM_0 for the MCP23017T
    err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_driver_install: I2C_NUM_%d %d (%s)", I2C_NUM_0, err, esp_err_to_name(err));
    }
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = TM_PIN_SDA;
    conf.scl_io_num = TM_PIN_SCL;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    err = i2c_param_config(I2C_NUM_0, &conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_param_config: I2C_NUM_%d %d (%s)", I2C_NUM_0, err, esp_err_to_name(err));
    }

    // initialize the mcp23017
    if (!mcp23017.begin())
    {
        ESP_LOGE(TAG, "failed to initialize the mcp23017!");
    }

    // initialize the display
    ESP_LOGI(TAG, "initializing the Display");
    if (!Display::getDisplay().begin())
    {
        ESP_LOGE(TAG, "failed to initialize the Display!");
    }

    new PageAbout();

    // turn the backlight on.
    ESP_LOGI(TAG,  "turning backlight on");
    mcp23017.portMode(MCP23017::B, 0, TM_BACKLIGHT_BIT);
    mcp23017.portWrite(MCP23017::B, TM_BACKLIGHT_BIT, TM_BACKLIGHT_BIT);

    if (!gps.begin(GPS_TX_PIN, GPS_RX_PIN))
    {
        ESP_LOGE(TAG, "failed to start gps!");
    }
}

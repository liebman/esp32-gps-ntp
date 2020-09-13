#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "TeaMakerPins.h"
#include "Display.h"
#include "MCP23017.h"
#include "PageAbout.h"

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
static QueueHandle_t gps_event_queue;

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

    // turn the backlight on.
    ESP_LOGI(TAG,  "turning backlight on");
    mcp23017.portMode(MCP23017::B, 0, TM_BACKLIGHT_BIT);
    mcp23017.portWrite(MCP23017::B, TM_BACKLIGHT_BIT, TM_BACKLIGHT_BIT);

    new PageAbout();


    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 10,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, nullptr, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    static uint8_t buffer[1024];
    size_t index = 0;
    buffer[index] = '\0';
    while (true)
    {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_1, &buffer[index], 1, pdMS_TO_TICKS(2000));
        if (len < 0)
        {
            ESP_LOGE(TAG, "uart read error!");
            continue;
        }
        if (len > 0)
        {
            // ignore LF
            if (buffer[index] == '\n')
            {
                buffer[index] = '\0';
                continue;
            }
            // display on CR (eat the CR)
            if (buffer[index] == '\r')
            {
                buffer[index] = '\0'; // eat CR
                ESP_LOGI(TAG, "GPS: %s", buffer);
                index = 0;
                buffer[index] = '\0';
                continue;
            }
            index += 1;
            buffer[index] = '\0';
            if (index >= sizeof(buffer)-1)
            {
                ESP_LOGI(TAG, "GPS (buffer full!!!!): %s", buffer);
                index = 0;
                buffer[index] = '\0';
            }
        }
    }
}

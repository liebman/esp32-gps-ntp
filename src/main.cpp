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

#define USE_PATTERN_MATCH

#define GPS_RX_PIN (GPIO_NUM_4)
#define GPS_TX_PIN (GPIO_NUM_2)

static const char* TAG = "app_main";
static MCP23017 mcp23017;
#ifdef USE_PATTERN_MATCH
static QueueHandle_t gps_event_queue;
#endif

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
#ifdef USE_PATTERN_MATCH
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 10, &gps_event_queue, 0);
#else
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, nullptr, 0);
#endif
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    static uint8_t buffer[1024];
#ifdef USE_PATTERN_MATCH
    uart_enable_pattern_det_baud_intr(UART_NUM_1, '\n', 1, 10000, 0, 0);
    uart_pattern_queue_reset(UART_NUM_1, 20);

    uart_event_t event;
    while (true)
    {
        if(xQueueReceive(gps_event_queue, (void * )&event, (portTickType)portMAX_DELAY) != pdTRUE)
        {
            ESP_LOGE(TAG, "failed to recieve gps event from uart event queue!");
        }

        switch(event.type)
        {
            case UART_DATA:
                break;

            case UART_FIFO_OVF:
                ESP_LOGE(TAG, "UART_FIFO_OVF!");
                uart_flush_input(UART_NUM_1);
                xQueueReset(gps_event_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGE(TAG, "UART_BUFFER_FULL!");
                uart_flush_input(UART_NUM_1);
                xQueueReset(gps_event_queue);
                break;
            case UART_BREAK:
                ESP_LOGI(TAG, "UART_BREAK");
                break;
            case UART_PARITY_ERR:
                ESP_LOGE(TAG, "UART_PARITY_ERR");
                break;
            case UART_DATA_BREAK:
                ESP_LOGE(TAG, "UART_DATA_BREAK");
                break;

            //Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGE(TAG, "UART_FRAME_ERR");
                break;
            case UART_EVENT_MAX:
            default:
                ESP_LOGI(TAG, "unknown uart event type: %d", event.type);
                break;

            case UART_PATTERN_DET:
                size_t size;
                uart_get_buffered_data_len(UART_NUM_1, &size);
                int pos = uart_pattern_pop_pos(UART_NUM_1);
                ESP_LOGD(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, size);
                if (pos == -1)
                {
                    // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                    // record the position. We should set a larger queue size.
                    // As an example, we directly flush the rx buffer here.
                    uart_flush_input(UART_NUM_1);
                }
                else if (pos > sizeof(buffer)-1)
                {
                    ESP_LOGE(TAG, "line too long for buffer: len=%d, buffer=%u", pos, sizeof(buffer)-1);
                    uart_flush_input(UART_NUM_1);
                }
                else
                {
                    int len = uart_read_bytes(UART_NUM_1, buffer, pos+1, 100 / portTICK_PERIOD_MS);
                    if (len != pos+1)
                    {
                        ESP_LOGE(TAG, "read size mismatch expected %d, got %d", pos+1, len);
                        uart_flush_input(UART_NUM_1);
                    }
                    else
                    {
                        buffer[len] = '\0';
                        // remove possible cr, lf
                        if (buffer[len-1] == '\r' || buffer[len-1] == '\n')
                        {
                            buffer[len-1] = '\0';
                        }
                        if (buffer[len-2] == '\r' || buffer[len-2] == '\n')
                        {
                            buffer[len-2] = '\0';
                        }
                        ESP_LOGI(TAG, "read data: %s", buffer);
                    }
                }
                break;


        }
    }
#else
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
#endif
}

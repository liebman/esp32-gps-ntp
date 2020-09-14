#include "GPS.h"
#include "minmea.h"
#include "esp_log.h"

static const char* TAG = "GPS";

#ifndef GPS_TASK_PRI
#define GPS_TASK_PRI 5
#endif

#ifndef GPS_TASK_CORE
#define GPS_TASK_CORE 1
#endif

typedef union {
    struct minmea_sentence_rmc rmc;
    struct minmea_sentence_gga gga;
    struct minmea_sentence_gsa gsa;
    struct minmea_sentence_gll gll;
    struct minmea_sentence_gst gst;
    struct minmea_sentence_gsv gsv;
    struct minmea_sentence_vtg vtg;
    struct minmea_sentence_zda zda;
} minmea_record_t;


GPS::GPS(uart_port_t uart_id, size_t buffer_size)
: _uart_id(uart_id),
  _buffer_size(buffer_size)
{
}

bool GPS::begin(gpio_num_t tx_pin, gpio_num_t rx_pin)
{
    _buffer = new char[_buffer_size];
    if (_buffer == nullptr)
    {
        ESP_LOGE(TAG, "::begin failed to allocate buffer!");
        return false;
    }

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 10,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(_uart_id, _buffer_size * 2, 0, 10, &_event_queue, 0);
    uart_param_config(_uart_id, &uart_config);
    uart_set_pin(_uart_id, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_enable_pattern_det_baud_intr(_uart_id, '\n', 1, 10000, 0, 0);
    uart_pattern_queue_reset(_uart_id, 10);

    xTaskCreatePinnedToCore(task, "GPS", 4096, this, GPS_TASK_PRI, &_task, GPS_TASK_CORE);

    return true;
}

// from GSV

int GPS::getSatsTotal()
{
    return _sats_total;
}

// from GGA

int GPS::getSatsTracked()
{
    return _sats_tracked;
}

int GPS::getFixQuality()
{
    return _fix_quality;
}

float GPS::getAltitude()
{
    return _altitude;
}

char GPS::getAltitudeUnits()
{
    return _altitude_units;
}

// from GSA

char GPS::getMode()
{
    return _mode;
}

int GPS::getFixType()
{
    return _fix_type;
}

// from RMC
bool GPS::getValid()
{
    return _valid;
}

float GPS::getLatitude()
{
    return _latitude;
}

float GPS::getLongitude()
{
    return _longitude;
}

void GPS::process(char* sentence)
{
    minmea_record_t data;

    switch (minmea_sentence_id(sentence, false))
    {
        case MINMEA_SENTENCE_RMC:
            if (!minmea_parse_rmc(&data.rmc, sentence))
            {
                ESP_LOGE(TAG, "$xxRMC sentence is not parsed");
                break;
            }

            _valid     = data.rmc.valid;
            _latitude  = minmea_tocoord(&data.rmc.latitude);
            _longitude = minmea_tocoord(&data.rmc.longitude);

            ESP_LOGI(TAG, "$xxRMC coordinates and speed: (%f,%f) %f",
                    minmea_tocoord(&data.rmc.latitude),
                    minmea_tocoord(&data.rmc.longitude),
                    minmea_tofloat(&data.rmc.speed));
            break;

        case MINMEA_SENTENCE_GGA:
            if (!minmea_parse_gga(&data.gga, sentence))
            {
                ESP_LOGE(TAG, "$xxGGA sentence is not parsed");
                break;
            }
            _fix_quality = data.gga.fix_quality;
            _sats_tracked = data.gga.satellites_tracked;
            _altitude = minmea_tofloat(&data.gga.altitude);
            _altitude_units = data.gga.altitude_units;
            ESP_LOGI(TAG, "$xxGGA: alt:%f '%c' fix quality: %d",
                            minmea_tofloat(&data.gga.altitude),
                            data.gga.altitude_units,
                            data.gga.fix_quality);

            break;

        case MINMEA_SENTENCE_GSA:
            if (!minmea_parse_gsa(&data.gsa, sentence))
            {
                ESP_LOGE(TAG, "$xxGSA sentence is not parsed");
                break;
            }
            _mode = data.gsa.mode;
            _fix_type = data.gsa.fix_type;

            ESP_LOGI(TAG, "$xxGSA: fix mode=%d type=%c", data.gsa.fix_type, data.gsa.mode);

            break;

        case MINMEA_SENTENCE_GLL:
            if (!minmea_parse_gll(&data.gll, sentence))
            {
                ESP_LOGE(TAG, "$xxGSA sentence is not parsed");
                break;
            }
            ESP_LOGI(TAG, "$xxGLL lat=%f lon=%f status=%c mode=%c",
                          minmea_tocoord(&data.gll.longitude),
                          minmea_tocoord(&data.gll.latitude),
                          data.gll.status, data.gll.mode);
            break;

        case MINMEA_SENTENCE_GST:
            ESP_LOGE(TAG, "::process sentence not implemented: '%s'", sentence);
            break;

        case MINMEA_SENTENCE_GSV:
            if (!minmea_parse_gsv(&data.gsv, sentence))
            {
                ESP_LOGE(TAG, "$xxGSV sentence is not parsed");
                break;
            }
            _sats_total = data.gsv.total_sats;
            ESP_LOGI(TAG, "$xxGSV: fix total_msgs=%d msg_nr=%d total_sats=%d", data.gsv.total_msgs, data.gsv.msg_nr, data.gsv.total_sats);
            break;

        case MINMEA_SENTENCE_VTG:
            if (!minmea_parse_vtg(&data.vtg, sentence)) 
            {
                ESP_LOGE(TAG, "$xxVTG sentence is not parsed");
                break;
            }
            ESP_LOGI(TAG, "$xxVTG: track true_deg=%f mag_deg=%f speed knots: %f kph: %f",
                            minmea_tofloat(&data.vtg.true_track_degrees),
                            minmea_tofloat(&data.vtg.magnetic_track_degrees),
                            minmea_tofloat(&data.vtg.speed_knots),
                            minmea_tofloat(&data.vtg.speed_kph));
            break;

        case MINMEA_SENTENCE_ZDA:
            if (!minmea_parse_zda(&data.zda, sentence))
            {
                ESP_LOGE(TAG, "$xxZDA sentence is not parsed");
                break;
            }

            ESP_LOGI(TAG, "$xxZDA: %d:%d:%d %02d.%02d.%d UTC%+03d:%02d",
                    data.zda.time.hours,
                    data.zda.time.minutes,
                    data.zda.time.seconds,
                    data.zda.date.day,
                    data.zda.date.month,
                    data.zda.date.year,
                    data.zda.hour_offset,
                    data.zda.minute_offset);
            break;

        case MINMEA_INVALID:
            ESP_LOGW(TAG, "::process sentence invalid: '%s'", sentence);
            break;

        case MINMEA_UNKNOWN:
            ESP_LOGW(TAG, "::process sentence unknown: '%s'", sentence);
            break;
    }
}

void GPS::task()
{
    uart_event_t event;
    while (true)
    {
        if(xQueueReceive(_event_queue, (void * )&event, (portTickType)portMAX_DELAY) != pdTRUE)
        {
            ESP_LOGE(TAG, "failed to recieve gps event from uart event queue!");
        }

        switch(event.type)
        {
            case UART_DATA:
                break;

            case UART_FIFO_OVF:
                ESP_LOGE(TAG, "::task got UART_FIFO_OVF!");
                uart_flush_input(_uart_id);
                xQueueReset(_event_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGE(TAG, "::task got UART_BUFFER_FULL!");
                uart_flush_input(_uart_id);
                xQueueReset(_event_queue);
                break;
            case UART_BREAK:
                ESP_LOGI(TAG, "::task got UART_BREAK");
                break;
            case UART_PARITY_ERR:
                ESP_LOGE(TAG, "::task got UART_PARITY_ERR");
                break;
            case UART_DATA_BREAK:
                ESP_LOGE(TAG, "::task got UART_DATA_BREAK");
                break;

            //Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGE(TAG, "::task got UART_FRAME_ERR");
                break;

            case UART_EVENT_MAX:
            default:
                ESP_LOGI(TAG, "::task got unknown uart event type: %d", event.type);
                break;

            case UART_PATTERN_DET:
                size_t size;
                uart_get_buffered_data_len(_uart_id, &size);
                int pos = uart_pattern_pop_pos(_uart_id);
                ESP_LOGD(TAG, "::task got UART_PATTERN_DET pos: %d, buffered size: %d", pos, size);
                if (pos == -1)
                {
                    // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                    // record the position. We should set a larger queue size.
                    // As an example, we directly flush the rx buffer here.
                    uart_flush_input(_uart_id);
                }
                else if (pos > _buffer_size-1)
                {
                    ESP_LOGE(TAG, "::task line too long for buffer: len=%d, buffer=%u", pos, _buffer_size-1);
                    uart_flush_input(_uart_id);
                }
                else
                {
                    int len = uart_read_bytes(_uart_id, (uint8_t*)_buffer, pos+1, 100 / portTICK_PERIOD_MS);
                    if (len != pos+1)
                    {
                        ESP_LOGE(TAG, "read size mismatch expected %d, got %d", pos+1, len);
                        uart_flush_input(_uart_id);
                    }
                    else
                    {
                        _buffer[len] = '\0';
                        // remove possible cr, lf
                        if (_buffer[len-1] == '\r' || _buffer[len-1] == '\n')
                        {
                            _buffer[len-1] = '\0';
                        }
                        if (_buffer[len-2] == '\r' || _buffer[len-2] == '\n')
                        {
                            _buffer[len-2] = '\0';
                        }

                        ESP_LOGD(TAG, "::task read data: %s", (char*)_buffer);
                        process(_buffer);
                    }
                }
                break;
        }
    }
}

void GPS::task(void* data)
{
    ESP_LOGI(TAG, "::task - starting!");
    GPS* gps = (GPS*)data;
    gps->task();
}

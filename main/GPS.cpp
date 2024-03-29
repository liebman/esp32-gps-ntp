/*
 * MIT License
 *
 * Copyright (c) 2020 Christopher B. Liebman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "GPS.h"
#include "string.h"
#include "minmea.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include <sys/time.h>
#include <soc/soc.h>

static const char* TAG = "GPS";

#ifndef GPS_TASK_PRI
#define GPS_TASK_PRI (configMAX_PRIORITIES/2)
#endif

#ifndef GPS_TASK_CORE
#define GPS_TASK_CORE 0
#endif

#define MINUTES2USECS(m) (m*60*1000000)

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


GPS::GPS(MicroSecondTimer& timer, uart_port_t uart_id, size_t buffer_size)
: _timer(timer),
  _uart_id(uart_id),
  _buffer_size(buffer_size),
  _lock(xSemaphoreCreateMutex())
{
}

bool GPS::begin(gpio_num_t tx_pin, gpio_num_t rx_pin)
{
    ESP_LOGI(TAG, "::begin allocating uart buffer");
    _buffer = new char[_buffer_size];
    if (_buffer == nullptr)
    {
        ESP_LOGE(TAG, "::begin failed to allocate buffer!");
        return false;
    }

    ESP_LOGI(TAG, "::begin configuring UART");
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

    ESP_LOGI(TAG, "::begin set UART pattern match on line terminator");
    uart_enable_pattern_det_baud_intr(_uart_id, '\n', 1, 10000, 0, 0);
    uart_pattern_queue_reset(_uart_id, 10);
#if CONFIG_GPSNTP_ENABLE_115220BAUD
#if CONFIG_GPSNTP_GPS_TYPE_SKYTRAQ
    const char* gpstype = "SKYTRAQ";
    uint8_t baudRateCmd[11] = {0xA0, 0xA1, 0x00, 0x04, 0x05, 0x00, 0x05, 0x00, 0x00, 0x0D, 0x0A}; // 115200 Baud Rate
    size_t baudRateSize = sizeof(baudRateCmd);
#elif CONFIG_GPSNTP_GPS_TYPE_UBLOX6M
    const char* gpstype = "UBLOX6M";
    uint8_t baudRateCmd[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x00, // id
        0x14, // length
        0x00, // 
        0x01, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xD0, // payload
        0x08, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xC2, // payload
        0x01, // payload
        0x00, // payload
        0x07, // payload
        0x00, // payload
        0x03, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        
        0xC0, // CK_A
        0x7E, // CK_B
    };
    size_t baudRateSize = sizeof(baudRateCmd);
#elif CONFIG_GPSNTP_GPS_TYPE_MTK3339
    const char* gpstype = "MTK3339";
    const uint8_t* baudRateCmd = (const uint8_t*)"$PMTK251,115200*1F\r\n";
    size_t baudRateSize = strlen((const char*)baudRateCmd);
#endif
    // update the baud rate to 115200
    ESP_LOGI(TAG, "::begin changing GPS baud rate to 115200 for %s", gpstype);
    uart_write_bytes(_uart_id, (char*)baudRateCmd, baudRateSize);
    vTaskDelay(pdMS_TO_TICKS(100));
    uart_set_baudrate(_uart_id, 115200);
    //uart_flush_input(_uart_id);
#endif

    ESP_LOGI(TAG, "::begin create GPS task at priority %d core %d", GPS_TASK_PRI, GPS_TASK_CORE);
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
bool GPS::getValid(uint32_t max_wait_ms)
{
    bool ret = false;
    if (xSemaphoreTake(_lock, pdMS_TO_TICKS(max_wait_ms)) == pdTRUE)
    {
        uint64_t now = _timer.getValue64();
        uint64_t age = now - _last_rmc;
        if (_valid && age > 1500000) // its bad if older than 1.5 seconds
        {
            ESP_LOGE(TAG, "::getValid returning false record too old %lluus  (now=%llu last=%llu)", age, now, _last_rmc);
            _valid = false;
            xSemaphoreGive(_lock);
            return false;
        }
        // return valid only if it has been valid for over 20 minutes
        // because in the first few minutes sometimes the time is off
        // by a couple of seconds even when GPS sends valid.
        age = now - _valid_since;
        if (age > MINUTES2USECS(20))
        {
            ret = _valid;
        }
        xSemaphoreGive(_lock);
    }
    else
    {
        ESP_LOGE(TAG, "::getValid failed to take lock, returning false!");
    }
    return ret;
}

uint32_t GPS::getValidDuration()
{
    return _valid ? (_timer.getValue64() - _valid_since) / 1000000 : 0;
}

uint32_t GPS::getValidCount()
{
    return _valid_count;
}

float GPS::getLatitude()
{
    return _latitude;
}

float GPS::getLongitude()
{
    return _longitude;
}

char* GPS::getPSTI()
{
    return _psti;
}

time_t GPS::getRMCTime()
{
    return _rmc_time.tv_sec;
}

time_t GPS::getZDATime()
{
    return _zda_time.tv_sec;
}

void GPS::process(char* sentence)
{
    minmea_record_t data;
    ESP_LOGV(TAG, "::process  '%s'", sentence);
    if (xSemaphoreTake(_lock, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        ESP_LOGE(TAG, "::process: failed to take lock, ignoring: '%s'", sentence);
        return;
    }
    switch (minmea_sentence_id(sentence, false))
    {
        case MINMEA_SENTENCE_RMC:
        {
            if (!minmea_parse_rmc(&data.rmc, sentence))
            {
                ESP_LOGE(TAG, "$xxRMC sentence is not parsed");
                break;
            }
            bool was_valid = _valid;
            _valid     = data.rmc.valid;
            _latitude  = minmea_tocoord(&data.rmc.latitude);
            _longitude = minmea_tocoord(&data.rmc.longitude);
            if (minmea_gettime(&_rmc_time, &data.rmc.date, &data.rmc.time))
            {
                _valid = false;
                ESP_LOGE(TAG, "::process RMC failed to convert date/time!");
            }
            _last_rmc = _timer.getValue64();
            if (_valid && !was_valid)
            {
                _valid_since = _last_rmc;
                _valid_count += 1;
                ESP_LOGI(TAG, "::process device reports valid!  count=%u", _valid_count);
            }
            else if (!_valid && was_valid)
            {
                ESP_LOGW(TAG, "::process device reports NOT valid!");
            }

            ESP_LOGD(TAG, "$xxRMC coordinates and speed: (%f,%f) %f",
                    minmea_tocoord(&data.rmc.latitude),
                    minmea_tocoord(&data.rmc.longitude),
                    minmea_tofloat(&data.rmc.speed));
            break;
        }

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
            ESP_LOGD(TAG, "$xxGGA: alt:%f '%c' fix quality: %d",
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

            ESP_LOGD(TAG, "$xxGSA: fix mode=%d type=%c", data.gsa.fix_type, data.gsa.mode);

            break;

        case MINMEA_SENTENCE_GLL:
            if (!minmea_parse_gll(&data.gll, sentence))
            {
                ESP_LOGE(TAG, "$xxGSA sentence is not parsed");
                break;
            }
            ESP_LOGD(TAG, "$xxGLL lat=%f lon=%f status=%c mode=%c",
                          minmea_tocoord(&data.gll.longitude),
                          minmea_tocoord(&data.gll.latitude),
                          data.gll.status, data.gll.mode);
            break;

        case MINMEA_SENTENCE_GST:
            ESP_LOGD(TAG, "::process sentence not implemented: '%s'", sentence);
            break;

        case MINMEA_SENTENCE_GSV:
            if (!minmea_parse_gsv(&data.gsv, sentence))
            {
                ESP_LOGE(TAG, "$xxGSV sentence is not parsed");
                break;
            }
            _sats_total = data.gsv.total_sats;
            ESP_LOGD(TAG, "$xxGSV: fix total_msgs=%d msg_nr=%d total_sats=%d", data.gsv.total_msgs, data.gsv.msg_nr, data.gsv.total_sats);
            break;

        case MINMEA_SENTENCE_VTG:
            // ignore empty VTG as it failes parsing
            if (strcmp(sentence, "$GPVTG,,,,,,,,,N*30") == 0)
            {
                break;
            }
            if (!minmea_parse_vtg(&data.vtg, sentence)) 
            {
                ESP_LOGE(TAG, "$xxVTG sentence is not parsed");
                break;
            }
            ESP_LOGD(TAG, "$xxVTG: track true_deg=%f mag_deg=%f speed knots: %f kph: %f",
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
            if (minmea_gettime(&_zda_time, &data.zda.date, &data.zda.time))
            {
                ESP_LOGE(TAG, "::process ZDA failed to convert date/time!");
            }

            ESP_LOGD(TAG, "$xxZDA: %d:%d:%d %02d.%02d.%d UTC%+03d:%02d",
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
            if (strncmp("$PSTI,00,", sentence, 9) == 0)
            {
                strncpy(_psti, sentence, sizeof(_psti)-1);
                // TODO: parse $PSTI,00 for not hide it if its timing mode 2
                break;
            }

            ESP_LOGW(TAG, "::process sentence invalid: '%s'", sentence);
            break;

        case MINMEA_UNKNOWN:
            ESP_LOGW(TAG, "::process sentence unknown: '%s'", sentence);
            break;
    }
    xSemaphoreGive(_lock);
}

void GPS::task()
{
    uart_event_t event;
    while (true)
    {
        if(xQueueReceive(_event_queue, (void * )&event, (portTickType)portMAX_DELAY) != pdTRUE)
        {
            ESP_LOGE(TAG, "failed to recieve gps event from uart event queue!");
            continue;
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
    ESP_LOGI(TAG, "::task Starting with priority %d core %d", uxTaskPriorityGet(nullptr), xPortGetCoreID());
    GPS* gps = static_cast<GPS*>(data);
    gps->task();
}

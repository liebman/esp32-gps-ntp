#include "GPS.h"
#include "string.h"
#include "minmea.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include <sys/time.h>

static const char* TAG = "GPS";

#ifndef GPS_TASK_PRI
#define GPS_TASK_PRI configMAX_PRIORITIES-1
#endif

#ifndef GPS_TASK_CORE
#define GPS_TASK_CORE 1
#endif

#ifndef GPS_TIMER_GROUP
#define GPS_TIMER_GROUP TIMER_GROUP_0
#endif

#ifndef GPS_TIMER_NUM
#define GPS_TIMER_NUM TIMER_0
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


GPS::GPS(gpio_num_t pps_pin, uart_port_t uart_id, size_t buffer_size)
: _pps_pin(pps_pin),
  _uart_id(uart_id),
  _buffer_size(buffer_size)
{
}

bool GPS::begin(gpio_num_t tx_pin, gpio_num_t rx_pin)
{
    // create PPS queue
    _pps_event_queue = xQueueCreate(20, sizeof(pps_event_msg_t));
    ESP_LOGI(TAG, "::begin allocating uart buffer");
    _buffer = new char[_buffer_size];
    if (_buffer == nullptr)
    {
        ESP_LOGE(TAG, "::begin failed to allocate buffer!");
        return false;
    }

    ESP_LOGI(TAG, "::begin configuring and starting timer");
    timer_config_t tc = {
        .alarm_en    = TIMER_ALARM_EN,
        .counter_en  = TIMER_PAUSE,
        .intr_type   = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_DIS,
        .divider     = 80,  // microseconds second
    };
    esp_err_t err = timer_init(GPS_TIMER_GROUP, GPS_TIMER_NUM, &tc);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::begin timer_init failed: group=%d timer=%d err=%d (%s)", GPS_TIMER_GROUP, GPS_TIMER_NUM, err, esp_err_to_name(err));
    }
    timer_set_counter_value(GPS_TIMER_GROUP, GPS_TIMER_NUM, 0x00000000ULL);
    timer_set_alarm_value(GPS_TIMER_GROUP, GPS_TIMER_NUM, 1005000);
    timer_enable_intr(GPS_TIMER_GROUP, GPS_TIMER_NUM);
    timer_isr_register(GPS_TIMER_GROUP, GPS_TIMER_NUM, timeoutISR,
                       (void *) this, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(GPS_TIMER_GROUP, GPS_TIMER_NUM);

    if (_pps_pin != GPIO_NUM_NC)
    {
        ESP_LOGI(TAG, "::begin configuring PPS pin %d", _pps_pin);
        gpio_set_direction(_pps_pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(_pps_pin, GPIO_PULLUP_ONLY);
        ESP_LOGI(TAG, "::begin setup ppsISR");
        gpio_set_intr_type(_pps_pin, GPIO_INTR_POSEDGE);
        gpio_intr_enable(_pps_pin);
        gpio_install_isr_service(0);
        //hook isr handler for specific gpio pin
        gpio_isr_handler_add(_pps_pin, ppsISR, this);
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

    ESP_LOGI(TAG, "::begin create GPSPPS task at priority %d core %d", GPS_TASK_PRI-1, GPS_TASK_CORE);
    xTaskCreatePinnedToCore(ppsEventLogger, "GPSPPS", 2048, this, GPS_TASK_PRI-1, nullptr, GPS_TASK_CORE);

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
            if (minmea_gettime(&_rmc_time, &data.rmc.date, &data.rmc.time))
            {
                ESP_LOGE(TAG, "::process RMC failed to convert date/time!");
            }

            if (_valid && _rmc_time.tv_sec != 0 && _rmc_time.tv_sec != _time && _pps_count > 2)
            {
                ESP_LOGE(TAG, "setting time from RMC (time:%ld != rmc:%ld)", _time, _rmc_time.tv_sec);
                _time = _rmc_time.tv_sec;
                struct timeval tv;
                uint64_t usecs;
                timer_set_counter_value(GPS_TIMER_GROUP, GPS_TIMER_NUM, 0x00000000ULL);
                timer_get_counter_value(GPS_TIMER_GROUP, GPS_TIMER_NUM, &usecs);
                tv.tv_sec = _time;
                tv.tv_usec = usecs;
                settimeofday(&tv, nullptr);
            }

            ESP_LOGD(TAG, "$xxRMC coordinates and speed: (%f,%f) %f",
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
#if 0
                if (strncmp("$PSTI,00,2,", sentence, 11) == 0)
                {
                    break;
                }
#else
                break;
#endif
            }

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
    ESP_LOGI(TAG, "::task - starting!");
    GPS* gps = (GPS*)data;
    gps->task();
}

uint32_t GPS::getPPSCount()
{
    return _pps_count;
}

uint32_t GPS::getPPSTimerMax()
{
    return _pps_timer_max;
}

uint32_t GPS::getPPSTimerMin()
{
    return _pps_timer_min;
}

uint32_t GPS::getPPSMissed()
{
    return _pps_missed;
}

uint32_t GPS::getPPSShort()
{
    return _pps_short;
}

void IRAM_ATTR GPS::pps()
{
    uint64_t current = timer_group_get_counter_value_in_isr(GPS_TIMER_GROUP, GPS_TIMER_NUM);
    // reset the timer
    TIMERG0.hw_timer[GPS_TIMER_NUM].load_high = 0;
    TIMERG0.hw_timer[GPS_TIMER_NUM].load_low  = 0;
    TIMERG0.hw_timer[GPS_TIMER_NUM].reload = 1;
    ++_pps_count;

    bool send = false;
    pps_event_msg_t msg {.type=PPS_SHORT, .value=(uint32_t)current};

    if (current < 900000)
    {
        ++_pps_short;
        msg.type = PPS_SHORT;
        send = true;
    }
    else
    {
        // only increment time if we are not on a short pps interrupt
        ++_time;

        if (current < _pps_timer_min)
        {
            _pps_timer_min = current;
            msg.type = PPS_MIN_UPDATE;
            send = true;
        }
    }

    if (current > _pps_timer_max)
    {
        _pps_timer_max = current;
        msg.type = PPS_MAX_UPDATE;
        send = true;
    }

    if (send)
    {
        BaseType_t yield = pdFALSE;
        xQueueSendFromISR(_pps_event_queue, &msg, &yield);
        if (yield)
        {
            portYIELD_FROM_ISR();
        }
    }
}

void IRAM_ATTR GPS::ppsISR(void* data)
{
    GPS* gps = (GPS*)data;
    gps->pps();
}

void IRAM_ATTR GPS::timeout()
{
    ++_pps_missed;
    _pps_timer_max = 0;
    timer_group_clr_intr_status_in_isr(GPS_TIMER_GROUP, GPS_TIMER_NUM);
    TIMERG0.hw_timer[GPS_TIMER_NUM].load_high = 0;
    TIMERG0.hw_timer[GPS_TIMER_NUM].load_low  = 0;
    TIMERG0.hw_timer[GPS_TIMER_NUM].reload = 1;
    timer_group_enable_alarm_in_isr(GPS_TIMER_GROUP, GPS_TIMER_NUM);

    pps_event_msg_t msg {.type=PPS_MISSED, .value=0};
    BaseType_t yield = pdFALSE;
    xQueueSendFromISR(_pps_event_queue, &msg, &yield);
    if (yield)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR GPS::timeoutISR(void* data)
{
    GPS* gps = (GPS*)data;
    gps->timeout();
}

void GPS::ppsEventLogger(void*data)
{
    GPS* gps = (GPS*)data;
    pps_event_msg_t event;
    while(true)
    {
        if(xQueueReceive(gps->_pps_event_queue, (void * )&event, (portTickType)portMAX_DELAY) != pdTRUE)
        {
            ESP_LOGE(TAG, "failed to recieve gps event from uart event queue!");
            continue;
        }
        const char* event_name = "<UNKNOWN>";
        switch (event.type)
        {
        case PPS_MISSED:
            event_name = "MISSED";
            break;

        case PPS_SHORT:
            event_name = "SHORT";
            break;

        case PPS_MIN_UPDATE:
            event_name = "MIN";
            break;

        case PPS_MAX_UPDATE:
            event_name = "MAX";
            break;

        default:
            break;
        }

        ESP_LOGW(TAG, "::ppsEventLogger %s:%07u tracked:%d quality:%d", event_name, event.value, gps->_sats_tracked, gps->_fix_quality);
    }
}

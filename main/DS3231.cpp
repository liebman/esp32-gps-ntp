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

#include "DS3231.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char* TAG = "DS3231";

static inline uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0f);
}

static inline uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

DS3231::DS3231(i2c_port_t i2c) : _i2c(i2c)
{
}

DS3231::~DS3231()
{
}

bool DS3231::begin()
{
    _lock = xSemaphoreCreateBinary();
    xSemaphoreGive(_lock);
    uint8_t data[REG_CNT];
    if (!read(SECONDS, REG_CNT, data))
    {
        ESP_LOGE(TAG, "failed to read all registers!");
        return false;
    }
    ESP_LOGI(TAG, "seconds: 0x%02x", data[SECONDS]);
    ESP_LOGI(TAG, "minutes: 0x%02x", data[MINUTES]);
    ESP_LOGI(TAG, "hours:   0x%02x", data[HOURS]);
    ESP_LOGI(TAG, "day:     0x%02x", data[DAY]);
    ESP_LOGI(TAG, "date:    0x%02x", data[DATE]);
    ESP_LOGI(TAG, "month:   0x%02x", data[MONTH]);
    ESP_LOGI(TAG, "year:    0x%02x", data[YEAR]);
    ESP_LOGI(TAG, "control: 0x%02x", data[CONTROL]);
    ESP_LOGI(TAG, "status:  0x%02x", data[STATUS]);
    ESP_LOGI(TAG, "aging:   %d",     (int8_t)data[AGEOFFSET]);

    _age_offset = (int8_t)data[AGEOFFSET];
#if 0
    //
    // if the age offset is outside a normal range then zero it!
    //  (30 is > 2ppm)
    if (abs(_age_offset) >= 30)
    {
        setAgeOffset(0);
    }
#endif

    updateReg(CONTROL, SQWAVE_1HZ, EOSC|BBSQW|SQWAVE_MASK|INTCN);
    updateReg(STATUS, 0, OSC_STOP_FLAG);
    updateReg(HOURS, 0, DS3231_12HR);

#if 0
    writeReg(AGEOFFSET, 0);
    updateReg(CONTROL, CONV, CONV);
#endif

    return true;
}

bool DS3231::getTime(struct tm *time)
{
    uint8_t data[7];

    if (!read(SECONDS, sizeof(data), data))
    {
        ESP_LOGE(TAG, "failed to read time from ds3231!");
        return false;
    }

    /* convert to unix time structure */
    time->tm_sec = bcd2dec(data[0]);
    time->tm_min = bcd2dec(data[1]);
    if (data[2] & DS3231_12HR)
    {
        /* 12H */
        time->tm_hour = bcd2dec(data[2] & DS3231_12HR_MASK) - 1;
        /* AM/PM? */
        if (data[2] & DS3231_PM) time->tm_hour += 12;
    }
    else time->tm_hour = bcd2dec(data[2]); /* 24H */
    time->tm_wday = bcd2dec(data[3]) - 1;
    time->tm_mday = bcd2dec(data[4]);
    time->tm_mon  = bcd2dec(data[5] & DS3231_MONTH_MASK) - 1;
    time->tm_year = bcd2dec(data[6]) + 100;
    time->tm_isdst = 0;

    // apply a time zone (if you are not using localtime on the rtc or you want to check/apply DST)
    //applyTZ(time);

    return true;
}

bool DS3231::setTime(struct tm* time)
{
    uint8_t data[7];

    /* time/date data */
    data[0] = dec2bcd(time->tm_sec);
    data[1] = dec2bcd(time->tm_min);
    data[2] = dec2bcd(time->tm_hour);
    /* The week data must be in the range 1 to 7, and to keep the start on the
     * same day as for tm_wday have it start at 1 on Sunday. */
    data[3] = dec2bcd(time->tm_wday + 1);
    data[4] = dec2bcd(time->tm_mday);
    data[5] = dec2bcd(time->tm_mon + 1);
    data[6] = dec2bcd(time->tm_year - 100);
    bool ret = write(SECONDS, sizeof(data), data);
    return ret;
}

int8_t DS3231::getAgeOffset()
{
    return _age_offset;
}

bool DS3231::setAgeOffset(int8_t ageoff)
{
    if (!writeReg(AGEOFFSET, (uint8_t)ageoff))
    {
        ESP_LOGE(TAG, "failed to write AGEOFFSET register!");
        return false;
    }

    _age_offset = ageoff;

    if (!updateReg(CONTROL, CONV, CONV))
    {
        ESP_LOGE(TAG, "failed to start conversion!");
        return false;
    }

    return true;
}

bool DS3231::updateReg(Register reg, uint8_t value, uint8_t mask)
{
    ESP_LOGD(TAG, "::updateReg: reg: 0x%02x value: 0x%02x mask: 0x%02x", reg, value, mask);
    bool ret = false;
    if (xSemaphoreTake( _lock, ( TickType_t ) 1000 ) == pdTRUE)
    {
        uint8_t            old = 0;
        if (mask != 0xff)
        {
            if (!readReg(reg, &old))
            {
                xSemaphoreGive(_lock);
                return false;
            }
            value = (value & mask) | (old & ~mask); // keep some old bits
        }
        ret = writeReg(reg, value);
        xSemaphoreGive(_lock);
    }
    return ret;
}

bool DS3231::writeReg(Register reg, uint8_t value)
{
    ESP_LOGD(TAG, "::writeReg: reg: 0x%02x value: 0x%02x", reg, value);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DS3231_ADDR << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(_i2c, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::writeReg i2c master command failed: %d (%s)", err, esp_err_to_name(err));
        return false;
    }

    return true;
}

bool DS3231::write(Register reg, size_t len, uint8_t* value)
{
    ESP_LOGD(TAG, "::write: reg: 0x%02x len: %u", reg, len);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DS3231_ADDR << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, value, len, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(_i2c, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::write i2c master command failed: %d (%s)", err, esp_err_to_name(err));
        return false;
    }

    return true;
}

bool DS3231::readReg(Register reg, uint8_t* value)
{
    ESP_LOGD(TAG, "::readReg: reg: 0x%02x", reg);
    bool ret = true;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DS3231_ADDR << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DS3231_ADDR << 1 | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, value, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(_i2c, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::readReg i2c master command failed: %d (%s)", err, esp_err_to_name(err));
        return false;
    }

    ESP_LOGD(TAG, "::readReg: value: 0x%02x", *value);
    return ret;
}

bool DS3231::read(Register reg, size_t len, uint8_t* value)
{
    ESP_LOGD(TAG, "::read: reg: 0x%02x len: %u", reg, len);
    bool ret = true;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DS3231_ADDR << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DS3231_ADDR << 1 | I2C_MASTER_READ, true);
    i2c_master_read(cmd, value, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(_i2c, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::read i2c master command failed: %d (%s)", err, esp_err_to_name(err));
        return false;
    }

    return ret;
}

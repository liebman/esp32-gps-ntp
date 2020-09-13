#include "MCP23017.h"
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char* TAG = "MCP23017";

MCP23017::MCP23017(i2c_port_t i2c)
: _i2c(i2c),
  _lock(nullptr)
{
}

MCP23017::~MCP23017()
{
}

bool MCP23017::begin(uint8_t addr)
{
    bool ret = true;
    _addr = addr;
    _lock = xSemaphoreCreateBinary();
    xSemaphoreGive(_lock);
    for(uint8_t i = 0; i < REG_CNT; ++i)
    {
        uint8_t value = (i <= IODIRB) ? 0xff : 0x0;
        if (!writeReg((Register)i, value))
        {
            ESP_LOGE(TAG, "::begin failed to write register 0x%02x", i);
            ret = false;
        }
    }
    return ret;
}

bool MCP23017::portMode(Port port, uint8_t mode, uint8_t mask)
{
    ESP_LOGD(TAG, "::portMode: port: 0x%02x mode: 0x%02x mask: 0x%02x", port, mode, mask);
    return updateReg((port == A) ? IODIRA : IODIRB, mode, mask);
}

bool MCP23017::portRead(Port port, uint8_t* value, uint8_t mask)
{
    ESP_LOGD(TAG, "::portRead: port: 0x%02x mask: 0x%02x", port, mask);
    bool ret = false;
    if (xSemaphoreTake( _lock, ( TickType_t ) 1000 ) == pdTRUE)
    {
        ret = readReg((port == A) ? GPIOA : GPIOB, value);
        *value &= mask;
        xSemaphoreGive(_lock);
        ESP_LOGD(TAG, "::portRead: value: 0x%02x", *value);
    }
    return ret;
}

bool MCP23017::portWrite(Port port, uint8_t value, uint8_t mask)
{
    ESP_LOGD(TAG, "::portWrite: port: 0x%02x value: 0x%02x mask: 0x%02x", port, value, mask);
    return updateReg((port == A) ? GPIOA : GPIOB, value, mask);
}

bool MCP23017::portPullUp(Port port, uint8_t value, uint8_t mask)
{
    ESP_LOGD(TAG, "::portPullUp: port: 0x%02x value: 0x%02x mask: 0x%02x", port, value, mask);
    return updateReg((port == A) ? GPPUA : GPPUB, value, mask);
}

bool MCP23017::updateReg(Register reg, uint8_t value, uint8_t mask)
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

bool MCP23017::writeReg(Register reg, uint8_t value)
{
    ESP_LOGD(TAG, "::writeReg: reg: 0x%02x value: 0x%02x", reg, value);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, _addr << 1 | I2C_MASTER_WRITE, true);
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

bool MCP23017::readReg(Register reg, uint8_t* value)
{
    ESP_LOGD(TAG, "::readReg: reg: 0x%02x", reg);
    bool ret = true;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, _addr << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, _addr << 1 | I2C_MASTER_READ, true);
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

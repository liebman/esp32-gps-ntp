#ifndef _DS3231_H
#define _DS3231_H

#include <driver/i2c.h>
#include <freertos/semphr.h>
#include <time.h>
#include "PPS.h"

class DS3231
{
public:
    enum SquareWave
    {
        SQWAVE_1HZ    = 0x00,
        SQWAVE_1024HZ = 0x08,
        SQWAVE_4096HZ = 0x10,
        SQWAVE_8192HZ = 0x18
    };

    DS3231(i2c_port_t i2c = I2C_NUM_0);
    virtual ~DS3231();
    bool begin();
    
    bool getTime(struct tm* tm);
    bool setTime(struct tm* tm);

    int8_t getAgeOffset();
    bool setAgeOffset(int8_t ageoff);

protected:
    const uint8_t DS3231_ADDR       = 0x68;
    const uint8_t DS3231_12HR       = 0x40;
    const uint8_t DS3231_PM         = 0x20;
    const uint8_t DS3231_12HR_MASK  = 0x1f;
    const uint8_t DS3231_MONTH_MASK = 0x1f;
    enum  Register : uint8_t
    {
        SECONDS    = 0x00,
        MINUTES    = 0x01,
        HOURS      = 0x02,
        DAY        = 0x03,
        DATE       = 0x04,
        MONTH      = 0x05,
        YEAR       = 0x06,
        A1_SECONDS = 0x07,
        A1_MINUTES = 0x08,
        A1_HOURS   = 0x09,
        A1_DAYDATE = 0x0a,
        A2_MINUTES = 0x0b,
        A2_HOURS   = 0x0c,
        A2_DAYDATE = 0x0d,
        CONTROL    = 0x0e,
        STATUS     = 0x0f,
        AGEOFFSET  = 0x10,
        TEMP_MSB   = 0x11,
        TEMP_LSB   = 0x12,
        REG_CNT,
    };

    enum Control : uint8_t
    {
        EOSC        = 0x80,
        BBSQW       = 0x40,
        CONV        = 0x20,
        SQWAVE_MASK = 0x18,
        INTCN       = 0x04,
        ALARM_2_EN  = 0x02,
        ALARM_1_EN  = 0x01
    };

    enum Status : uint8_t
    {
        OSC_STOP_FLAG = 0x80,
        EN32KHZ       = 0x08,
        BUSY          = 0x04,
        ALARM_2_FLAG  = 0x02,
        ALARM_1_FLAG  = 0x01
    };

    bool updateReg(Register reg, uint8_t value, uint8_t mask);
    bool writeReg(Register reg, uint8_t value);
    bool write(Register reg, size_t len, uint8_t* value);
    bool readReg(Register reg, uint8_t* value);
    bool read(Register reg, size_t len, uint8_t* value);
private:
    i2c_port_t         _i2c;
    SemaphoreHandle_t  _lock = nullptr;
    int8_t             _age_offset = 0;
};

#endif // _DS3231_H

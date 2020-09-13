#ifndef _MCP23017_H
#define _MCP23017_H
#include <driver/i2c.h>
#include <freertos/semphr.h>

class MCP23017
{
public:
    static const uint8_t DEFAULT_ADDR = 0x20;
    enum Port
    {
        A = 0,
        B = 1
    };

    MCP23017(i2c_port_t i2c = I2C_NUM_0);
    virtual ~MCP23017();
    bool begin(uint8_t addr = DEFAULT_ADDR);
    bool portMode(Port port, uint8_t mode, uint8_t mask = 0xff);
    bool portRead(Port port, uint8_t* value, uint8_t mask = 0xff);
    bool portWrite(Port port, uint8_t value, uint8_t mask = 0xff);
    bool portPullUp(Port port, uint8_t value, uint8_t mask = 0xff);

protected:
    enum  Register : uint8_t
    {
        IODIRA   = 0x00,
        IODIRB   = 0x01,
        IPOLA    = 0x02,
        IPOLB    = 0x03,
        GPINTENA = 0x04,
        GPINTENB = 0x05,
        DEFVALA  = 0x06,
        DEFVALB  = 0x07,
        INTCONA  = 0x08,
        INTCONB  = 0x09,
        IOCONA   = 0x0a,
        IOCONB   = 0x0b,
        GPPUA    = 0x0c,
        GPPUB    = 0x0d,
        INTFA    = 0x0e,
        INTFB    = 0x0f,
        INTCAPA  = 0x10,
        INTCAPB  = 0x11,
        GPIOA    = 0x12,
        GPIOB    = 0x13,
        OLATA    = 0x14,
        OLATB    = 0x15,
        REG_CNT
    };

    bool updateReg(Register reg, uint8_t value, uint8_t mask);
    bool writeReg(Register reg, uint8_t value);
    bool readReg(Register reg, uint8_t* value);

private:
    i2c_port_t         _i2c;
    uint8_t            _addr;
    SemaphoreHandle_t  _lock;
};

#endif // _MCP23017_H

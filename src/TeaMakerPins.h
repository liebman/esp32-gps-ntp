#ifndef TEA_MAKER_PINS_H_
#define TEA_MAKER_PINS_H_

#include <stdint.h>

const uint8_t TM_PIN_ONEWIRE = 26;

#ifdef OLD_V8_BOARD
#define BACKLIGHT_PIN GPIO_NUM_25
#define BACKLIGHT_SEL GPIO_SEL_25
#else
const uint8_t TM_BACKLIGHT_BIT  = 0b10000000;
#endif // OLD_V8_BOARD

#define TOUCH_IRQ_PIN GPIO_NUM_36
#define TOUCH_IRQ_SEL GPIO_SEL_36

// first I2C buss (internal) "Wire"
const uint8_t TM_PIN_SDA  = 16;
const uint8_t TM_PIN_SCL  = 17;
const uint8_t TM_MCP23017T_ADDR = 0x20;
#define MCP23017_RST_PIN GPIO_NUM_27
#define MCP23017_RST_SEL GPIO_SEL_27

// second I2C buss (external) "Wire1"
const uint8_t TM_PIN_SDA1 = 33;
const uint8_t TM_PIN_SCL1 = 32;
const uint8_t TM_GRIDEYE_ADDR = 0x69;

const uint8_t TM_MOTOR1_SHIFT   = 4;
const uint8_t TM_MOTOR1_MASK    = 0b11110000; // bits controling motor 2
const uint8_t TM_MOTOR1_LL_MASK = 0b00000001;
const uint8_t TM_MOTOR1_UL_MASK = 0b00000010;

const uint8_t TM_MOTOR2_SHIFT   = 0;
const uint8_t TM_MOTOR2_MASK    = 0b00001111; // bits controling motor 1
const uint8_t TM_MOTOR2_LL_MASK = 0b00000100;
const uint8_t TM_MOTOR2_UL_MASK = 0b00001000;

#endif // TEA_MAKER_PINS_H_

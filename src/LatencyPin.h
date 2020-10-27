#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

#ifdef LATENCY_OUTPUT
#define LATENCY_PIN (GPIO_NUM_2)
#define LATENCY_SEL (GPIO_SEL_2)
#endif


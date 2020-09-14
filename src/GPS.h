#ifndef _GPS_H
#define _GPS_H
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

class GPS
{
public:
    GPS(uart_port_t uart_id = UART_NUM_1, size_t buffer_size = 1024);
    bool begin(gpio_num_t tx_pin, gpio_num_t rx_pin);

protected:

    uart_port_t _uart_id;
    size_t      _buffer_size;
    uint8_t*    _buffer;

private:
    QueueHandle_t _event_queue;
    TaskHandle_t  _task;

    void task();
    static void task(void* data);
};

#endif // _LINE_READER_H

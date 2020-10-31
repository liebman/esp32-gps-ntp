#ifndef _GPS_H
#define _GPS_H
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "minmea.h"
#include <functional>
#include "PPS.h"

class GPS
{
    
public:
    GPS(uart_port_t uart_id = UART_NUM_1, size_t buffer_size = 2048);
    bool  begin(gpio_num_t tx_pin, gpio_num_t rx_pin);
    // from GSV
    int   getSatsTotal();
    // from GGA
    int   getSatsTracked();
    int   getFixQuality();
    float getAltitude();
    char  getAltitudeUnits();
    // from GSA
    char  getMode();
    int   getFixType();
    // from RMC
    bool  getValid();
    float getLatitude();
    float getLongitude();
    char* getPSTI();
    time_t getRMCTime();
    time_t getZDATime();
    void setTime(std::function<void(time_t time)>);

protected:
    uart_port_t _uart_id;
    size_t      _buffer_size;
    char*       _buffer;
    // from GSV
    int         _sats_total;
    //from GGA
    int         _sats_tracked;
    int         _fix_quality;
    float       _altitude;
    char        _altitude_units;
    // from GSA
    char        _mode;
    int         _fix_type;
    // from RMC
    bool        _valid;
    float       _latitude;
    float       _longitude;
    char        _psti[81];

    struct timespec _rmc_time;
    struct timespec _zda_time;

private:
    QueueHandle_t _event_queue;
    TaskHandle_t  _task;
    intr_handle_t _timer_int;

    void process(char* sentence);
    void task();
    static void task(void* data);

};

#endif // _GPS_H

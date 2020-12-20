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
#include "MicroSecondTimer.h"

class GPS
{
    
public:
    GPS(MicroSecondTimer& timer, uart_port_t uart_id = UART_NUM_1, size_t buffer_size = 2048);
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
    bool  getValid(uint32_t max_wait_ms=10);
    uint64_t getValidSince();
    float getLatitude();
    float getLongitude();
    char* getPSTI();
    time_t getRMCTime();
    time_t getZDATime();

protected:
    MicroSecondTimer& _timer;
    uart_port_t _uart_id;
    size_t      _buffer_size;
    char*       _buffer;
    // from GSV
    volatile int         _sats_total;
    //from GGA
    volatile int         _sats_tracked;
    volatile int         _fix_quality;
    volatile float       _altitude;
    volatile char        _altitude_units;
    // from GSA
    volatile char        _mode;
    volatile int         _fix_type;
    // from RMC
    volatile bool        _valid;
    volatile float       _latitude;
    volatile float       _longitude;
    char        _psti[81];
    struct timespec _rmc_time;
    volatile uint64_t    _last_rmc;
    volatile uint64_t    _valid_since;

    // from ZDA if present
    struct timespec _zda_time;

private:
    SemaphoreHandle_t _lock;
    QueueHandle_t _event_queue;
    TaskHandle_t  _task;

    void process(char* sentence);
    void task();
    static void task(void* data);

};

#endif // _GPS_H

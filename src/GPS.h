#ifndef _GPS_H
#define _GPS_H
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "minmea.h"


class GPS
{
    
public:
    GPS(gpio_num_t pps_pin = GPIO_NUM_NC, uart_port_t uart_id = UART_NUM_1, size_t buffer_size = 2048);
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
    uint32_t getPPSCount();
    uint32_t getPPSTimerMax();
    uint32_t getPPSTimerMin();
    uint32_t getPPSMissed();
    uint32_t getPPSShort();
    uint32_t getPPSLast();

protected:
    gpio_num_t  _pps_pin;
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

    volatile uint32_t _pps_count      = 0;
    volatile uint32_t _pps_missed     = 0;
    volatile uint32_t _pps_short      = 0;
    volatile uint32_t _pps_timer_max  = 0;
    volatile uint32_t _pps_timer_min  = 2000000;
    volatile uint32_t _pps_last; // last pps in usecs even if short or timeout
    volatile time_t   _time; // unix seconds

    struct timespec _rmc_time;
    struct timespec _zda_time;

private:
    QueueHandle_t _event_queue;
    TaskHandle_t  _task;
    intr_handle_t _timer_int;

    void process(char* sentence);
    void task();
    static void task(void* data);
    static void pps(void* data);
    static void timeout(void* data);
};

#endif // _LINE_READER_H

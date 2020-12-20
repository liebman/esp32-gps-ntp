#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "LVTabView.h"

class Display
{
public:
    static Display& getDisplay();
    ~Display();
    bool begin();
    LVPage* newPage(const char* name);
    bool lock(int timeout_ms); // negitive timeout is wait forever
    void unlock();
    uint32_t getIdleTime();
private:
    Display();
    SemaphoreHandle_t _lock;
    LVTabView*        _tabview;
    lv_disp_t*        _disp;
    static void task(void* data);
    static void tickAction(void* data);
};
#endif // _DISPLAY_H

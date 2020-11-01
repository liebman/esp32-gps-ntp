#ifndef _NETWORK_H
#define _NETWORK_H
#include "freertos/FreeRTOS.h"
#include "esp_event.h"

class Network
{
public:
    enum NetworkStatus
    {
        CONNECTED = BIT0,
    };
    static Network& getNetwork();
    ~Network();
    bool begin();

private:
    Network();
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

#endif // _NETWORK_H

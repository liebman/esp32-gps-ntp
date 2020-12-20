#ifndef _NETWORK_H
#define _NETWORK_H
#include "freertos/FreeRTOS.h"
#include "esp_event.h"

class Network
{
public:
    using Status = uint32_t;
    static const Status HAS_IP       = BIT0;

    static Network& getNetwork();
    ~Network();
    bool begin(const char* ssid, const char* password);
    bool hasIP();
    esp_ip4_addr_t getIPAddress(char* buf = nullptr, size_t size = 0);
    uint32_t waitFor(Status status, TickType_t wait = portMAX_DELAY);

private:
    Network();
    esp_ip4_addr_t _ip;
    char           _ip_str[16];
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

#endif // _NETWORK_H

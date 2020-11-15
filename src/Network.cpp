#include "Network.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "memory.h"

static const char* TAG = "Network";
static EventGroupHandle_t network_status;

Network::Network()
{
}

Network& Network::getNetwork()
{
    static Network* network;
    if (network == nullptr)
    {
        network = new Network();
    }
    return *network;
}

bool Network::begin(const char* ssid, const char* password)
{
    ESP_LOGI(TAG, "::begin starting");
    network_status = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Network::eventHandler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Network::eventHandler, this));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    for(uint8_t i = 0; ssid[i] != 0; ++i)
    {
        wifi_config.sta.ssid[i] = ssid[i];
    }
    for(uint8_t i = 0; password[i] != 0; ++i)
    {
        wifi_config.sta.password[i] = password[i];
    }
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable  = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // disable power save as it adds up to 300ms or more in latemcy!
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "::begin finished");

    return true;
}

bool Network::hasIP()
{
    EventBits_t bits = xEventGroupWaitBits(network_status, HAS_IP, pdFALSE, pdFALSE, 0);
    return (bits & HAS_IP) == HAS_IP;
}

esp_ip4_addr_t Network::getIPAddress(char* buf, size_t size)
{
    if (buf != nullptr && size != 0)
    {
        strncpy(buf, _ip_str, size);
    }
    return _ip;
}

uint32_t Network::waitFor(Status status, TickType_t wait)
{
    return xEventGroupWaitBits(network_status, status, pdFALSE, pdFALSE, wait);
}

void Network::eventHandler(void* data, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    Network* net = (Network*)data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        xEventGroupClearBits(network_status, HAS_IP);
        memset(&net->_ip, 0, sizeof(net->_ip));
        net->_ip_str[0] = '\0';
        esp_wifi_connect();
        ESP_LOGI(TAG, "station was started, initiated connect to AP");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(network_status, HAS_IP);
        memset(&net->_ip, 0, sizeof(net->_ip));
        net->_ip_str[0] = '\0';
        esp_wifi_connect();
        ESP_LOGI(TAG, "disconnected! retry to connect to the AP");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        net->_ip = event->ip_info.ip;
        snprintf(net->_ip_str, sizeof(net->_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(network_status, HAS_IP);
        ESP_LOGI(TAG, "got ip: %s", net->_ip_str);
    }
}

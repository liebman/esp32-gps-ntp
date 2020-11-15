#ifndef _CONFIG_H
#define _CONFIG_H

#include "nvs_flash.h"
#include <string>

class Config {
public:
    Config();
    ~Config();
    bool begin(const char* partition_name = "config");

    bool load();
    bool save();

    void setWiFiSSID(const char* ssid);
    const char* getWiFiSSID();
    void setWiFiPassword(const char* password);
    const char* getWiFiPassword();
    void setBias(float bias);
    float getBias();
private:
    nvs_handle_t _nvs;
    char*        _wifi_ssid = nullptr;
    char*        _wifi_pass = nullptr;
    float        _bias = 0.0;
    char* getString(const char* key, const char* def_value = "");
    void  getString(const char* key, const char** valuep, const char* def_value = "");
    float getFloat(const char* key, float def_value = 0.0);
    char* copyString(const char* str);
};

#endif // _CONFIG_H

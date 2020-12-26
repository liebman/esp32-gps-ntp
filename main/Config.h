/*
 * MIT License
 *
 * Copyright (c) 2020 Christopher B. Liebman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

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
    void setTarget(float target);
    float getTarget();
private:
    nvs_handle_t _nvs;
    char*        _wifi_ssid = nullptr;
    char*        _wifi_pass = nullptr;
    float        _bias = 0.0;
    float        _target = 10.0;
    char* getString(const char* key, const char* def_value = "");
    void  getString(const char* key, const char** valuep, const char* def_value = "");
    float getFloat(const char* key, float def_value = 0.0);
    char* copyString(const char* str);
};

#endif // _CONFIG_H

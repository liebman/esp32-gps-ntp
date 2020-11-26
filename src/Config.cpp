#include "Config.h"
#include "esp_log.h"
#include "string.h"

static const char* TAG           = "Config";
static const char* KEY_WIFI_SSID = "wifi_ssid";
static const char* KEY_WIFI_PASS = "wifi_pass";
static const char* KEY_BIAS      = "bias";
static const char* KEY_TARGET    = "target";

Config::Config()
{
    setWiFiSSID("");
    setWiFiPassword("");
}

Config::~Config()
{
}

bool Config::begin(const char* partition_name)
{
    ESP_LOGI(TAG, "::begin()");
    esp_err_t err = nvs_flash_init_partition(partition_name);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_LOGI(TAG, "call nvs_flash_erase_partition '%s'", partition_name);
        err = nvs_flash_erase_partition(partition_name);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "failed nvs_flash_erase_partition '%s' nvs: %d (%s)", partition_name, err, esp_err_to_name(err));
            return false;
        }
        ESP_LOGI(TAG, "call nvs_flash_init_partition '%s'", partition_name);
        err = nvs_flash_init_partition(partition_name);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "failed nvs_flash_init_partition '%s' nvs: %d (%s)", partition_name, err, esp_err_to_name(err));
            return false;
        }
    }

    ESP_LOGI(TAG, "opening NVS partition '%s'", partition_name);
    err = nvs_open(partition_name, NVS_READWRITE, &_nvs);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to open '%s' nvs: %d (%s)", partition_name, err, esp_err_to_name(err));
        return false;
    }
    return true;
}

void Config::getString(const char* key, const char** valuep, const char* def_value)
{
    if (*valuep)
    {
        delete[] *valuep;
    }
    *valuep = def_value;

    size_t len;
    esp_err_t err = nvs_get_str(_nvs, key, nullptr, &len);
    if (err != ESP_OK)
    {
        return;
    }
    char* value = new char[len+1];
    nvs_get_str(_nvs, key, value, &len);
    *valuep = value;
}

char* Config::getString(const char* key, const char* def_value)
{
    size_t len;
    esp_err_t err = nvs_get_str(_nvs, key, nullptr, &len);
    if (err != ESP_OK)
    {
        return copyString(def_value);
    }
    char* value = new char[len+1];
    nvs_get_str(_nvs, key, value, &len);
    return value;
}

char* Config::copyString(const char* str)
{
    if (str == nullptr)
    {
        return nullptr;
    }
    size_t len = strlen(str);
    char* value = new char[len+1];
    strcpy(value, str);
    return value;
}

float Config::getFloat(const char* key, float def_value)
{
    size_t len;
    esp_err_t err = nvs_get_blob(_nvs, key, nullptr, &len);
    if (err != ESP_OK)
    {
        return def_value;
    }

    if (len != sizeof(float))
    {
        ESP_LOGE(TAG, "wrong size for value of '%s' %d != %d", key, len, sizeof(float));
        return def_value;
    }
    float value;
    nvs_get_blob(_nvs, key, &value, &len);
    return value;
}

bool Config::load()
{
    ESP_LOGI(TAG, "::load()");
    if (_wifi_ssid != nullptr)
    {
        delete[] _wifi_ssid;
    }
    _wifi_ssid = getString(KEY_WIFI_SSID);

    if (_wifi_pass != nullptr)
    {
        delete[] _wifi_pass;
    }
    _wifi_pass = getString(KEY_WIFI_PASS);

    _bias = getFloat(KEY_BIAS);

    _target = getFloat(KEY_TARGET);

    ESP_LOGI(TAG, "::load: ssid=%s pass=%s bias=%0f target=%0f", _wifi_ssid, _wifi_pass, _bias, _target);
    return true;
}

bool Config::save()
{
    ESP_LOGI(TAG, "::save()");
    bool ret = true;
    esp_err_t err = nvs_set_str(_nvs, KEY_WIFI_SSID, _wifi_ssid);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::save failed to set '%s=%s': %d (%s)", KEY_WIFI_SSID, _wifi_ssid, err, esp_err_to_name(err));
        ret = false;
    }
    err = nvs_set_str(_nvs, KEY_WIFI_PASS, _wifi_pass);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::save failed to set '%s=%s': %d (%s)", KEY_WIFI_PASS, _wifi_pass, err, esp_err_to_name(err));
        ret = false;
    }
    err = nvs_set_blob(_nvs, KEY_BIAS, &_bias, sizeof(_bias));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::save failed to set '%s=%f': %d (%s)", KEY_BIAS, _bias, err, esp_err_to_name(err));
        ret = false;
    }
    err = nvs_set_blob(_nvs, KEY_TARGET, &_target, sizeof(_target));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "::save failed to set '%s=%f': %d (%s)", KEY_TARGET, _target, err, esp_err_to_name(err));
        ret = false;
    }
    return ret;
}

void Config::setWiFiSSID(const char* ssid)
{
    if (_wifi_ssid != nullptr)
    {
        delete[] _wifi_ssid;
    }
    _wifi_ssid = copyString(ssid);
}

const char* Config::getWiFiSSID()
{
    if (_wifi_ssid == nullptr)
    {
        return "";
    }
    return _wifi_ssid;
}

void Config::setWiFiPassword(const char* password)
{
    if (_wifi_pass != nullptr)
    {
        delete[] _wifi_pass;
    }
    _wifi_pass = copyString(password);
}

const char* Config::getWiFiPassword()
{
    if (_wifi_pass == nullptr)
    {
        return "";
    }
    return _wifi_pass;
}

void Config::setBias(float bias)
{
    _bias = bias;
}

float Config::getBias()
{
    return _bias;
}

void Config::setTarget(float target)
{
    _target = target;
}

float Config::getTarget()
{
    return _target;
}

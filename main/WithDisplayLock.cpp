#include "WithDisplayLock.h"
#include "Display.h"
#include "esp_log.h"

static const char* TAG = "WithDisplayLock";

WithDisplayLock::WithDisplayLock(std::function<void()> got_lock, std::function<void()> got_no_lock)
{
    _locked = Display::getDisplay().lock(1000);
    if (_locked)
    {
        ESP_LOGD(TAG, "calling got_lock exists=%d", got_lock != nullptr);
        got_lock();
    }
    else if (got_no_lock)
    {
        ESP_LOGD(TAG, "got_no_lock!");
        got_no_lock();
    }
}

WithDisplayLock::WithDisplayLock()
{
    while(true)
    {
        _locked = Display::getDisplay().lock(1000);
        if (_locked)
        {
            return;
        }
        ESP_LOGE(TAG, "failed to get lock, will retry!");
    }
}

WithDisplayLock::~WithDisplayLock()
{
    if (_locked)
    {
        ESP_LOGD(TAG, "unlocking!");
        Display::getDisplay().unlock();
    }
}

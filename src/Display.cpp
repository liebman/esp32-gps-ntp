#include "Display.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lvgl/lvgl.h"
#include "lvgl_helpers.h"

#include "LVLabel.h"

#ifndef DISPLAY_TASK_PRI
#define DISPLAY_TASK_PRI 1
#endif

#ifndef DISPLAY_LV_TICK_MS
#define DISPLAY_LV_TICK_MS 1
#endif

#ifndef DISPLAY_TASK_CORE
#define DISPLAY_TASK_CORE 0
#endif

static const char* TAG = "Display";

Display& Display::getDisplay()
{
    static Display* display;
    if (display == nullptr)
    {
        display = new Display();
    }

    return *display;
}

Display::Display()
{
}

bool Display::begin()
{
    // there is only one Display object so this "should" be safe.
    _lock = xSemaphoreCreateRecursiveMutex();

    ESP_LOGI(TAG,  "calling: lv_init");
    lv_init();
    
    /* Initialize SPI or I2C bus used by the drivers */
    ESP_LOGI(TAG,  "calling: lvgl_driver_init");
    lvgl_driver_init();

    /* Use double buffered when not working with monochrome displays */
    static lv_color_t buf1[DISP_BUF_SIZE];
#ifndef CONFIG_LVGL_TFT_DISPLAY_MONOCHROME
    static lv_color_t buf2[DISP_BUF_SIZE];
#endif

    static lv_disp_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

    /* Initialize the working buffer depending on the selected display */

    ESP_LOGI(TAG,  "calling: lv_disp_buf_init");
#if defined CONFIG_LVGL_TFT_DISPLAY_CONTROLLER_IL3820 \
    || defined CONFIG_LVGL_TFT_DISPLAY_CONTROLLER_JD79653A \
    || defined CONFIG_LVGL_TFT_DISPLAY_CONTROLLER_UC8151D
    /* Actual size in pixel, not bytes and use single buffer */
    size_in_px *= 8;
    lv_disp_buf_init(&disp_buf, buf1, NULL, size_in_px);
#elif defined CONFIG_LVGL_TFT_DISPLAY_MONOCHROME
    lv_disp_buf_init(&disp_buf, buf1, NULL, size_in_px);
#else
    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);
#endif

    ESP_LOGI(TAG,  "calling: lv_disp_drv_init");
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

    /* When using a monochrome display we need to register the callbacks:
     * - rounder_cb
     * - set_px_cb */
#ifdef CONFIG_LVGL_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif

    ESP_LOGI(TAG,  "calling: lv_disp_drv_register");
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Register an input device when enabled on the menuconfig */
#if CONFIG_LVGL_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    ESP_LOGI(TAG,  "calling: lv_indev_drv_init");
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    ESP_LOGI(TAG,  "calling: lv_indev_drv_register");
    lv_indev_drv_register(&indev_drv);
#endif

    // quite these
    esp_log_level_set("XPT2046",    ESP_LOG_WARN);
    esp_log_level_set("spi_master", ESP_LOG_WARN);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    ESP_LOGI(TAG,  "starting lv_tick_action timer");
    const esp_timer_create_args_t periodic_timer_args = {
        .callback        = &Display::tickAction,
        .arg             = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "lv_tick_action"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, DISPLAY_LV_TICK_MS*1000)); // (param is microseconds)

    ESP_LOGI(TAG, "Creating tabview");
    _tabview = new LVTabView(nullptr);
    _tabview->setAnimationTime(0);

    ESP_LOGI(TAG, "Creating style for tabview");
    static LVStyle style_tab_btn;
    style_tab_btn.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
    _tabview->addStyle(LV_TABVIEW_PART_TAB_BTN , &style_tab_btn);

    xTaskCreatePinnedToCore(task, "Display", 4096*2, NULL, DISPLAY_TASK_PRI, NULL, DISPLAY_TASK_CORE);
    return true;
}

Display::~Display()
{
}

LVPage* Display::newPage(const char* name)
{
    ESP_LOGI(TAG, "::newPage name='%s'", name);
    LVPage* page = nullptr;
    lock(-1000);
    page = _tabview->addTab(name);
    unlock();
    return page;
}

void Display::task(void* data)
{
    ESP_LOGI(TAG, "::task starting!");
    while(true)
    {
        Display::getDisplay().lock(-1000);
        lv_task_handler(); /* let the GUI do its work */
        Display::getDisplay().unlock();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Display::tickAction(void *arg) {
    (void) arg;     // unused
    lv_tick_inc(DISPLAY_LV_TICK_MS);
}

bool Display::lock(int timeout_ms)
{
    if (!_lock)
    {
        ESP_LOGE(TAG, "::lock _lock not initialized!");
        return false;
    }

    if (timeout_ms < 0)
    {
        while (xSemaphoreTakeRecursive( _lock, pdMS_TO_TICKS(-timeout_ms) ) != pdTRUE)
        {
            ESP_LOGE(TAG, "::lock failed to take lock within 1 second, retrying");
        }
        return true;
    }

    return xSemaphoreTakeRecursive( _lock, pdMS_TO_TICKS(timeout_ms) ) == pdTRUE;
}

void Display::unlock()
{
    xSemaphoreGiveRecursive(_lock);
}
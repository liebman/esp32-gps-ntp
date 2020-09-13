#include "PageAbout.h"
#include "Display.h"
#include "WithDisplayLock.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageAbout";

PageAbout::PageAbout()
{
    WithDisplayLock([this](){
        _page = Display::getDisplay().newPage("About");
        _free = new LVLabel(_page);
        _free->align(nullptr, LV_ALIGN_CENTER, 0, 0);

        ESP_LOGI(TAG, "creating About task");
        lv_task_create(task, 1000, LV_TASK_PRIO_LOW, this);
    });
}

PageAbout::~PageAbout()
{
}

void PageAbout::task(lv_task_t *task)
{
    PageAbout* p = (PageAbout*)task->user_data;
    p->update();
}

void PageAbout::update()
{
    char buf[24];
    snprintf(buf, sizeof(buf)-1, "Free: %u", esp_get_free_heap_size());
    _free->setText(buf);
    _free->realign();
}

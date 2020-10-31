#include "PagePPS.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PagePPS";

PagePPS::PagePPS(PPS& gps_pps)
: _gps_pps(gps_pps)
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        _page = Display::getDisplay().newPage("PPS");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_LEFT);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _gps_time     = new LVLabel(cont);
        _gps_minmax   = new LVLabel(cont);
        _gps_shortlong = new LVLabel(cont);
    
        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 100, LV_TASK_PRIO_LOW, this);
    });
}

PagePPS::~PagePPS()
{
}

void PagePPS::task(lv_task_t *task)
{
    PagePPS* p = (PagePPS*)task->user_data;
    p->update();
}

static void fmtTime(const char* label, char* result, const size_t size, const time_t time, const uint32_t microseconds)
{
    char buf[256];
    struct tm tm;
    gmtime_r(&time, &tm);
    strftime(buf, sizeof(buf)-1, "%Y:%m:%d %H:%M:%S", &tm);
    snprintf(result, size, "%s%s.%02u", label, buf, microseconds/10000);
}

void PagePPS::update()
{
    static char buf[128];
    uint32_t microseconds;
    time_t   time;
    
    time = _gps_pps.getTime(&microseconds);
    fmtTime("GPS PPS: ", buf, sizeof(buf)-1, time, microseconds);
    _gps_time->setText(buf);

    snprintf(buf, sizeof(buf)-1, "Min/Max: %06u / %07u",
            _gps_pps.getTimerMin(),_gps_pps.getTimerMax());
    _gps_minmax->setText(buf);

    snprintf(buf, sizeof(buf)-1, "Short/Long: %u / %u",
            _gps_pps.getTimerShort(), _gps_pps.getTimerLong());
    _gps_shortlong->setText(buf);
}
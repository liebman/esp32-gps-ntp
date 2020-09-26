#include "PageGPS.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageGPS";

PageGPS::PageGPS(GPS& gps) : _gps(gps)
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        _page = Display::getDisplay().newPage("GPS");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_LEFT);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _rmc_time = new LVLabel(cont);
        _sats     = new LVLabel(cont);
        _status   = new LVLabel(cont);
        _pos      = new LVLabel(cont);
        _pps      = new LVLabel(cont);
        _psti     = new LVLabel(cont);
    
        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 100, LV_TASK_PRIO_LOW, this);
    });
}

PageGPS::~PageGPS()
{
}

void PageGPS::task(lv_task_t *task)
{
    PageGPS* p = (PageGPS*)task->user_data;
    p->update();
}

void PageGPS::update()
{
    char buf[80];
    snprintf(buf, sizeof(buf)-1, "Sats: %d tracked: %d", _gps.getSatsTotal(), _gps.getSatsTracked());
    _sats->setText(buf);

    snprintf(buf, sizeof(buf)-1, "Valid: %s mode: %c type: %d quality: %d",
            _gps.getValid() ? "yes" : "no",
            _gps.getMode(), _gps.getFixType(), _gps.getFixQuality());
    _status->setText(buf);

    snprintf(buf, sizeof(buf)-1, "lat: %f lon:%f alt: %0.1f%c", _gps.getLatitude(), _gps.getLongitude(), _gps.getAltitude(), _gps.getAltitudeUnits());
    _pos->setText(buf);
    time_t time = _gps.getRMCTime();
    const char* rt = ctime_r(&time, buf);
    if (rt == nullptr)
    {
        strncpy(buf, "<date/time unknown>", sizeof(buf)-1);
    }
    else
    {
        buf[strlen(buf)-1] = '\0';
    }
    _rmc_time->setText(buf);

    snprintf(buf, sizeof(buf)-1, "PPS: %u missed=%d short=%u\nmin=%f\nmax=%f\nlast=%f",
                                 _gps.getPPSCount(), _gps.getPPSMissed(), _gps.getPPSShort(),
                                (double)_gps.getPPSTimerMin()/1000000.0,
                                (double)_gps.getPPSTimerMax()/1000000.0,
                                (double)_gps.getPPSLast()/1000000.0);
    _pps->setText(buf);

    _psti->setText(_gps.getPSTI());
}

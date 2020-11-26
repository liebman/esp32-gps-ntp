#include "PageNTP.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageSync";

enum Row
{
    PPS_TIME,
    REQUESTS,
    RESPONSES,
    _NUM_ROWS
};

PageNTP::PageNTP(NTP& ntp, SyncManager& syncman)
: _ntp(ntp),
  _syncman(syncman)
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        _page = Display::getDisplay().newPage("NTP");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_LEFT);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _table = new LVTable(cont);
        _table->addStyle(LV_TABLE_PART_BG, &_container_style);
        _table->addStyle(LV_TABLE_PART_CELL1, &_container_style);
        _table->setColumnCount(2);
        _table->setColumnWidth(0, 80);
        _table->setColumnWidth(1, 160);
        _table->setRowCount(Row::_NUM_ROWS);

        _table->setCellAlign(Row::PPS_TIME, 0, LV_LABEL_ALIGN_RIGHT);
        _table->setCellAlign(Row::REQUESTS, 0, LV_LABEL_ALIGN_RIGHT);
        _table->setCellAlign(Row::RESPONSES, 0, LV_LABEL_ALIGN_RIGHT);

        _table->setCellValue(Row::PPS_TIME, 0, "Time:");
        _table->setCellValue(Row::REQUESTS, 0, "Req:");
        _table->setCellValue(Row::RESPONSES, 0, "Resp:");

        _table->setCellAlign(Row::PPS_TIME, 1, LV_LABEL_ALIGN_LEFT);
        _table->setCellAlign(Row::REQUESTS, 1, LV_LABEL_ALIGN_LEFT);
        _table->setCellAlign(Row::RESPONSES, 1, LV_LABEL_ALIGN_LEFT);

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 100, LV_TASK_PRIO_LOW, this);
    });
}

PageNTP::~PageNTP()
{
}

void PageNTP::task(lv_task_t *task)
{
    PageNTP* p = (PageNTP*)task->user_data;
    p->update();
}

static void fmtTime(const char* label, char* result, const size_t size, const time_t time, const uint32_t microseconds)
{
    char buf[256];
    struct tm tm;
    gmtime_r(&time, &tm);
    strftime(buf, sizeof(buf)-1, "%Y:%m:%d %H:%M:%S", &tm);
    if (microseconds < 1000000)
    {
        snprintf(result, size, "%s%s.%02u", label, buf, microseconds/10000);
    }
    else
    {
        strncpy(result, buf, size);
    }
}

void PageNTP::update()
{
    static char buf[128];
    struct timeval tv;
    _syncman.getRTCPPSTime(&tv);
    fmtTime("", buf, sizeof(buf)-1, tv.tv_sec, tv.tv_usec);
    _table->setCellValue(Row::PPS_TIME, 1, buf);


    snprintf(buf, sizeof(buf)-1, "%u", _ntp.getRequests());
    _table->setCellValue(Row::REQUESTS, 1, buf);

    snprintf(buf, sizeof(buf)-1, "%u", _ntp.getResponses());
    _table->setCellValue(Row::RESPONSES, 1, buf);
}

#ifndef _PAGE_NTP_H_
#define _PAGE_NTP_H_

#include "NTP.h"
#include "SyncManager.h"
#include "LVPage.h"
#include "LVTable.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PageNTP {
public:
    PageNTP(NTP& ntp, SyncManager& syncman);
    ~PageNTP();
private:
    void update();
    static void task(lv_task_t* task);
    NTP&         _ntp;
    SyncManager& _syncman;
    LVPage*      _page;
    LVTable*     _table;
    LVStyle      _container_style;
};

#endif // _PAGE_NTP_H_

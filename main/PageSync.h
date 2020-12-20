#ifndef _PAGE_SYNC_H_
#define _PAGE_SYNC_H_

#include "SyncManager.h"
#include "LVPage.h"
#include "LVTable.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PageSync {
public:
    PageSync(SyncManager& syncman);
    ~PageSync();
private:
    void update();
    static void task(lv_task_t* task);
    SyncManager& _syncman;
    LVPage*  _page;
    LVTable* _table;
    LVStyle  _container_style;
};

#endif // _PAGE_SYNC_H_

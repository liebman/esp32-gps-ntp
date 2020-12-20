#ifndef _PAGE_TASK_H_
#define _PAGE_TASK_H_

#include "SyncManager.h"
#include "LVPage.h"
#include "LVTable.h"
#include "LVLabel.h"
#include "LVStyle.h"

class PageTask {
public:
    PageTask();
    ~PageTask();
private:
    void update();
    static void task(lv_task_t* task);
    LVPage*  _page;
    LVTable* _table;
    LVStyle  _container_style;
};

#endif // _PAGE_TASK_H_

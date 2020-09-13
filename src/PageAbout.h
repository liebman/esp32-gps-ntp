#ifndef _PAGE_ABOUT_H_
#define _PAGE_ABOUT_H_
#include "LVPage.h"
#include "LVLabel.h"

class PageAbout {
public:
    PageAbout();
    ~PageAbout();
private:
    void update();
    static void task(lv_task_t* task);
    LVPage*  _page;
    LVLabel* _free;
};

#endif // _PAGE_ABOUT_H_

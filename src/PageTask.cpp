#include "PageTask.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageTask";


PageTask::PageTask()
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        _page = Display::getDisplay().newPage("Task");
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

        _table->setColumnCount(3);
        _table->setColumnWidth(0, 80);
        _table->setColumnWidth(1, 160);
        _table->setColumnWidth(1, 80);
        _table->setRowCount(10);

        ESP_LOGI(TAG, "creating task");
        lv_task_create(task, 10000, LV_TASK_PRIO_LOW, this);
    });
}

PageTask::~PageTask()
{
}

void PageTask::task(lv_task_t *task)
{
    PageTask* p = (PageTask*)task->user_data;
    p->update();
}


void PageTask::update()
{
    char buf[64];
    UBaseType_t count = uxTaskGetNumberOfTasks(); // can't be 0 as at least the Displat task has to be running
    TaskStatus_t* tasks = (TaskStatus_t*)pvPortMalloc( count * sizeof( TaskStatus_t ) );
    UBaseType_t n_tasks = uxTaskGetSystemState( tasks, count, nullptr );
    _table->setRowCount(n_tasks);
    for(UBaseType_t i = 0, row = 0; i < n_tasks; ++i)
    {
        if (tasks[i].xCoreID == 0)
        {
            continue;
        }
        _table->setCellValue(row, 0, tasks[i].pcTaskName);
        snprintf(buf, sizeof(buf)-1, "%u/%u", tasks[i].uxCurrentPriority, tasks[i].uxBasePriority);
        _table->setCellValue(row, 1, buf);
        snprintf(buf, sizeof(buf)-1, "%d", tasks[i].xCoreID);
        _table->setCellValue(row, 2, buf);
        ++row;
        //ESP_LOGI(TAG, "%u: %s: pri=%u/%u core=%d", i, tasks[i].pcTaskName, tasks[i].uxCurrentPriority, tasks[i].uxBasePriority, tasks[i].xCoreID);
    }
    vPortFree(tasks);
}

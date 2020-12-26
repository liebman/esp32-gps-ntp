/*
 * MIT License
 *
 * Copyright (c) 2020 Christopher B. Liebman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "PageAbout.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"
#include "Network.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "PageAbout";

PageAbout::PageAbout()
{
    WithDisplayLock([this](){
        _container_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style.setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style.setShadowWidth(LV_STATE_DEFAULT, 0);

        _page = Display::getDisplay().newPage("About");
        _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_container_style);

        LVContainer* cont = new LVContainer(_page);
        cont->setFit(LV_FIT_PARENT/*, LV_FIT_TIGHT*/);
        cont->addStyle(LV_CONT_PART_MAIN, &_container_style);
        cont->setLayout(LV_LAYOUT_COLUMN_LEFT);
        cont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
        cont->setDragParent(true);

        _free    = new LVLabel(cont);
        _address = new LVLabel(cont);
        _uptime = new LVLabel(cont);

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
    char buf[64];
    snprintf(buf, sizeof(buf)-1, "Free: %u", esp_get_free_heap_size());
    _free->setText(buf);
    char addr[24];
    Network::getNetwork().getIPAddress(addr, sizeof(addr));
    snprintf(buf, sizeof(buf)-1, "Address: %s", addr);
    _address->setText(buf);

    uint32_t seconds = esp_timer_get_time() / 1000000; // uptime in seconds
    uint32_t days     = seconds / 86400;
    seconds          -= days * 86400;
    uint32_t hours    = seconds /3600;
    seconds          -= hours * 3600;
    uint32_t minutes  = seconds / 60;
    seconds          -= minutes * 60;
    snprintf(buf, sizeof(buf)-1, "Uptime: %dd %02dh %02dm %02ds", days, hours, minutes, seconds);
    _uptime->setText(buf);
}

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

#include "PageConfig.h"
#include "Display.h"
#include "WithDisplayLock.h"
#include "LVContainer.h"
#include "LVLabel.h"
#include "esp_log.h"

static const char* TAG = "PageConfig";

static void configTask(lv_task_t* task)
{
    PageConfig* p = static_cast<PageConfig*>(task->user_data);
    p->process();
}

PageConfig::PageConfig(Config& config, const ApplyCB& apply_cb)
: _config(config),
  _apply_cb(apply_cb)
{
    WithDisplayLock lock; // this creates a lock that unlocks when destroyed

    _page = Display::getDisplay().newPage("Cfg");
    _contrainer_style.setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
    _contrainer_style.setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
    _contrainer_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
    _contrainer_style.setBorderWidth(LV_STATE_DEFAULT, 0);
    _contrainer_style.setShadowWidth(LV_STATE_DEFAULT, 0);

    _button_style.setPad(LV_STATE_DEFAULT, LV_DPX(4), LV_DPX(4), LV_DPX(4), LV_DPX(4));
    _button_style.setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);

    _page->addStyle(LV_PAGE_PART_SCROLLABLE, &_contrainer_style);

    LVContainer* cont = new LVContainer(_page);
    cont->setFit(LV_FIT_PARENT, LV_FIT_TIGHT);
    cont->addStyle(LV_CONT_PART_MAIN, &_contrainer_style);
    cont->setLayout(LV_LAYOUT_COLUMN_MID);
    cont->align(nullptr, LV_ALIGN_IN_TOP_MID, 0, 0);
    cont->setDragParent(true);

    LVContainer* fcont = new LVContainer(cont);
    fcont->setFit(LV_FIT_TIGHT);
    fcont->addStyle(LV_CONT_PART_MAIN, &_contrainer_style);
    fcont->setLayout(LV_LAYOUT_COLUMN_LEFT);
    fcont->align(nullptr, LV_ALIGN_CENTER, 0, 0);
    fcont->setDragParent(true);

    _ssid = new FieldText(fcont, "SSID:", 32,
            [this](){
                _config.setWiFiSSID(_ssid->getText());
            },
            [this](){
                _ssid->setText(_config.getWiFiSSID());
            });
    _ssid->setText(_config.getWiFiSSID());

    _password = new FieldText(fcont, "PSK:", 32,
            [this](){
                _config.setWiFiPassword(_password->getText());
            },
            [this](){
                _password->setText(_config.getWiFiPassword());
            });
    _password->setPasswordMode(true);
    _password->setText(_config.getWiFiPassword());

    _bias = new FieldText(fcont, "Bias:", 32,
            [this](){
                _config.setBias(atof(_bias->getText()));
            },
            [this](){
                char buf[32];
                snprintf(buf, sizeof(buf)-1, "%0f", _config.getBias());
                _bias->setText(buf);
            });
    char buf[32];
    snprintf(buf, sizeof(buf)-1, "%0f", _config.getBias());
    _bias->setText(buf);
    _bias->setKeyboardMode(LV_KEYBOARD_MODE_NUM);

    _target = new FieldText(fcont, "Target:", 32,
            [this](){
                _config.setTarget(atof(_target->getText()));
            },
            [this](){
                char buf[32];
                snprintf(buf, sizeof(buf)-1, "%0f", _config.getTarget());
                _target->setText(buf);
            });
    snprintf(buf, sizeof(buf)-1, "%0f", _config.getTarget());
    _target->setText(buf);
    _target->setKeyboardMode(LV_KEYBOARD_MODE_NUM);

    LVContainer* ctrls = new LVContainer(cont);
    ctrls->setFit(LV_FIT_TIGHT);
    ctrls->setLayout(LV_LAYOUT_ROW_MID);
    ctrls->align(nullptr, LV_ALIGN_CENTER, 0, 0);
    ctrls->addStyle(LV_CONT_PART_MAIN, &_contrainer_style);
    ctrls->setStylePadInner(LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 50);
    ctrls->setStyleMarginTop(LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 10);
    ctrls->setDragParent(true);

    _load = makeButton(ctrls, "LOAD", [this](lv_event_t event){
        if(event == LV_EVENT_CLICKED) {
            ESP_LOGI(TAG, "_load CB: loading");
            if (!_config.load())
            {
                ESP_LOGE(TAG, "_load CB: loading failed!");
            }
            _ssid->setText(_config.getWiFiSSID());
            _password->setText(_config.getWiFiPassword());
            char buf[32];
            snprintf(buf, sizeof(buf)-1, "%0f", _config.getBias());
            _bias->setText(buf);
            snprintf(buf, sizeof(buf)-1, "%0f", _config.getTarget());
            _target->setText(buf);
        }
    });

    _save = makeButton(ctrls, "SAVE", [this](lv_event_t event){
        if(event == LV_EVENT_CLICKED) {
            ESP_LOGI(TAG, "_save CB: saving!");
            _config.save();
        }
    });

    _apply = makeButton(ctrls, "APPLY", [this](lv_event_t event){
        if(event == LV_EVENT_CLICKED) {
            ESP_LOGI(TAG, "_apply CB: invoke callback!!");
            if (_apply_cb)
            {
                _apply_cb();
            }
        }
    });

    ESP_LOGI(TAG, "init: creating lv task");
    lv_task_create(configTask, 100, LV_TASK_PRIO_LOW, this);
}

PageConfig::~PageConfig()
{
}

LVButton* PageConfig::makeButton(LVBase* parent, const char* label, LVBase::LVEventCB cb)
{
    LVButton* btn = new LVButton(parent);
    btn->addStyle(LV_BTN_PART_MAIN, &_button_style);
    btn->setFit(LV_FIT_TIGHT, LV_FIT_TIGHT);
    btn->setEventCB(cb);
    LVLabel* lbl = new LVLabel(btn);
    lbl->setText(label);
    return btn;
}

LVSpinBox* PageConfig::makeSpinBoxField(LVBase* parent, const char* label, int32_t initial_value, LVBase::LVEventCB cb)
{
    LVContainer* cont = new LVContainer(parent);
    cont->addStyle(LV_CONT_PART_MAIN, &_contrainer_style);
    cont->setFit(LV_FIT_TIGHT);
    cont->setLayout(LV_LAYOUT_ROW_MID);
    cont->setDragParent(true);

    LVLabel* l = new LVLabel(cont);
    l->setStaticText(label);
    l->setDragParent(true);

    LVButton* inc = new LVButton(cont);
    inc->applyTheme(LV_THEME_SPINBOX_BTN);
    inc->setStyleValueStr(LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_SYMBOL_PLUS);

    LVSpinBox* sb = new LVSpinBox(cont);
    sb->setEventCB(cb);
    ESP_LOGI(TAG, "makeSpinBoxField - initial value %d", initial_value);
    sb->setValue(initial_value);

    LVButton* dec = new LVButton(cont);
    dec->applyTheme(LV_THEME_SPINBOX_BTN);
    dec->setStyleValueStr(LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_SYMBOL_MINUS);

    lv_coord_t h = sb->getHeight();
    inc->setSize(h, h);
    dec->setSize(h, h);

    inc->setEventCB([this,sb](lv_event_t event){
        if (event == LV_EVENT_SHORT_CLICKED || event == LV_EVENT_LONG_PRESSED_REPEAT)
        {
            sb->increment();
        }
    });
    dec->setEventCB([this,sb](lv_event_t event){
        if (event == LV_EVENT_SHORT_CLICKED || event == LV_EVENT_LONG_PRESSED_REPEAT)
        {
            sb->decrement();
        }
    });

    return sb;
}

void PageConfig::process()
{
}

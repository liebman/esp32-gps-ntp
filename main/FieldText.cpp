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

#include "FieldText.h"
#include "esp_log.h"

static const char* TAG = "FieldText";

LVStyle* FieldText::_container_style;
LVStyle* FieldText::_textarea_style;
LVKeyboard* FieldText::_keyboard;
std::function<void(void)> FieldText::_kb_submit_action;
std::function<void(void)> FieldText::_kb_cancel_action;

FieldText::FieldText(LVBase* parent, const char* label, uint16_t length, const std::function<void(void)> submit, const std::function<void(void)> cancel)
{
    if (_container_style == nullptr)
    {
        _container_style = new LVStyle();
        _container_style->setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _container_style->setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _container_style->setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _container_style->setBorderWidth(LV_STATE_DEFAULT, 0);
        _container_style->setShadowWidth(LV_STATE_DEFAULT, 0);
    }

    if (_textarea_style == nullptr)
    {
        _textarea_style = new LVStyle();
        _textarea_style->setPadInner(LV_STATE_DEFAULT, LV_DPX(2));
        _textarea_style->setPad(LV_STATE_DEFAULT, LV_DPX(1), LV_DPX(1), LV_DPX(1), LV_DPX(1));
        _textarea_style->setMargin(LV_STATE_DEFAULT, 0, 0, 0, 0);
        _textarea_style->setBorderWidth(LV_STATE_DEFAULT, 1);
        _textarea_style->setShadowWidth(LV_STATE_DEFAULT, 0);
    }

    if (_keyboard == nullptr)
    {
        _keyboard = new LVKeyboard(nullptr);
        _keyboard->setCurserManage(true);
        _keyboard->setHidden(true);
        _keyboard->setX(0);
        _keyboard->setEventCB([this](lv_event_t event) {
            _keyboard->defaultEventCB(event);
            //ESP_LOGI(TAG, "_keyboard event: %s", _keyboard->getEventName(event));
            switch (event)
            {
            case LV_EVENT_APPLY:
                if (_kb_submit_action)
                {
                    _kb_submit_action();
                    _kb_submit_action = nullptr;
                }
                _keyboard->setTextArea(nullptr);
                _keyboard->setHidden(true);
                break;

            case LV_EVENT_CANCEL:
                if (_kb_cancel_action)
                {
                    _kb_cancel_action();
                    _kb_cancel_action = nullptr;
                }
                _keyboard->setTextArea(nullptr);
                _keyboard->setHidden(true);
                break;
            
            default:
                break;
            }
        });
    }

    _cont = new LVContainer(parent);
    _cont->addStyle(LV_CONT_PART_MAIN, _container_style);
    _cont->setFit(LV_FIT_TIGHT);
    //_cont->setLayout(LV_LAYOUT_ROW_MID);
    _cont->setDragParent(true);

    _label = new LVLabel(_cont);
    _label->setStaticText(label);
    _label->setDragParent(true);
    _label->align(_cont, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    _textarea = new LVTextArea(_cont);
    _textarea->addStyle(LV_TEXTAREA_PART_BG, _textarea_style);
    _textarea->setOneLine(true);
    _textarea->setMaxLength(length);
    _textarea->setPlaceholderText("");
    _textarea->setText("");
    _textarea->setCurserHidden(true);
    _textarea->align(_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    _textarea->setEventCB([this, submit, cancel](lv_event_t event){
        switch (event)
        {
        case LV_EVENT_PRESSED:
            ESP_LOGI(TAG, "keyboard points to '%s'", _label->getText());
            // setup the kb actions to cancle or save
            _kb_submit_action = submit;
            _kb_cancel_action = cancel;
            _keyboard->setMode(_mode);
            _keyboard->setTextArea(_textarea);
            _keyboard->setParent(_cont);
            _keyboard->alignY(_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0);
            _keyboard->setHidden(false);
            break;
        default:
            break;
        }
    });
}

void FieldText::setText(const char* text)
{
    _textarea->setText(text);
}

const char* FieldText::getText()
{
    return _textarea->getText();
}

void FieldText::setPasswordMode(bool en)
{
    _textarea->setPasswordMode(en);
}

void FieldText::setKeyboardMode(lv_keyboard_mode_t mode)
{
    _mode = mode;
}

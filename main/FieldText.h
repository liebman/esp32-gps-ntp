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

#ifndef _FIELD_TEXT_H_
#define _FIELD_TEXT_H_

#include "LVStyle.h"
#include "LVContainer.h"
#include "LVLabel.h"
#include "LVTextArea.h"
#include "LVKeyboard.h"

class FieldText {
public:
    FieldText(LVBase* parent, const char* label, uint16_t length, std::function<void(void)> submit, std::function<void(void)> cancel);
    ~FieldText();

    void setText(const char* text);
    const char* getText();
    void setPasswordMode(bool en);
    void setKeyboardMode(lv_keyboard_mode_t mode);

private:
    LVContainer* _cont;
    LVLabel*     _label;
    LVTextArea*  _textarea;
    lv_keyboard_mode_t _mode = LV_KEYBOARD_MODE_TEXT_LOWER;
    static LVStyle* _container_style;
    static LVStyle* _textarea_style;
    static LVKeyboard* _keyboard;
    static std::function<void(void)> _kb_submit_action;
    static std::function<void(void)> _kb_cancel_action;
};

#endif // _FIELD_TEXT_H_

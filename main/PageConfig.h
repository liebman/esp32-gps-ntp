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

#ifndef _PAGE_CONFIG_H
#define _PAGE_CONFIG_H

#include "Config.h"
#include "LVPage.h"
#include "LVButton.h"
#include "LVContainer.h"
#include "LVSpinBox.h"
#include "LVTextArea.h"
#include "LVKeyboard.h"
#include "FieldText.h"

class PageConfig
{
public:
    using ApplyCB = std::function<void()>;
    explicit PageConfig(Config& config, const ApplyCB& apply_cb = nullptr);
    ~PageConfig();
    void process();

    PageConfig(PageConfig&) = delete;
    PageConfig& operator=(PageConfig&) = delete;

private:
    Config&         _config;
    const ApplyCB&  _apply_cb;

    LVStyle         _contrainer_style;
    LVStyle         _button_style;

    LVPage*         _page;
    FieldText*      _ssid;
    FieldText*      _password;
    FieldText*      _bias;
    FieldText*      _target;
    LVButton*       _apply;
    LVButton*       _save;
    LVButton*       _load;

    LVButton*       makeButton(LVBase* parent, const char* label, LVBase::LVEventCB cb);
    LVSpinBox*      makeSpinBoxField(LVBase* parent, const char* label, int32_t initial_value, LVBase::LVEventCB cb);
};
#endif // _PAGE_CONFIG_H

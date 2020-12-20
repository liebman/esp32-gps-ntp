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
    PageConfig(Config& config, ApplyCB apply_cb = nullptr);
    ~PageConfig();
    void process();

private:
    Config&    _config;
    ApplyCB    _apply_cb;

    LVStyle    _contrainer_style;
    LVStyle    _button_style;

    LVPage*    _page;
    FieldText* _ssid;
    FieldText* _password;
    FieldText* _bias;
    FieldText* _target;
    LVButton*  _apply;
    LVButton*  _save;
    LVButton*  _load;

    LVButton*   makeButton(LVBase* parent, const char* label, LVBase::LVEventCB cb);
    LVSpinBox*  makeSpinBoxField(LVBase* parent, const char* label, int32_t initial_value, LVBase::LVEventCB cb);
};
#endif // _PAGE_CONFIG_H

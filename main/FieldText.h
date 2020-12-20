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

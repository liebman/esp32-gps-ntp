#ifndef _WITH_DISPLAY_LOCK_H
#define _WITH_DISPLAY_LOCK_H
#include <functional>

class WithDisplayLock {
public:
    WithDisplayLock(std::function<void()> got_lock, std::function<void()> got_no_lock = nullptr);
    WithDisplayLock();
    ~WithDisplayLock();

private:
    bool _locked = false;
};

#endif // _WITH_DISPLAY_LOCK_H

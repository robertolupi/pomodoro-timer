#pragma once

#include <mutex>

class SPILock
{
public:
    SPILock();
    ~SPILock();

private:
    static std::recursive_mutex mutex_;
};

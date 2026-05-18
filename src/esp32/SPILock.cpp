#include "SPILock.h"

std::recursive_mutex SPILock::mutex_;

SPILock::SPILock()
{
    mutex_.lock();
}

SPILock::~SPILock()
{
    mutex_.unlock();
}

#pragma once
#include <mutex>

extern std::recursive_mutex spi_mutex;

bool ensureSDMounted();

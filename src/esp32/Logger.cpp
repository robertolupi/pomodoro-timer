#include "Logger.h"
#include "Global.h"
#include <SD.h>
#include <Arduino.h>

String isoformat_ctime(const time_t* t)
{
    struct tm* timeinfo = localtime(t);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}

void Logger::log_pomodoro(time_t start, time_t end, uint8_t flavor) {
    if (!ensureSDMounted())
    {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(spi_mutex);
    File file = SD.open(FILENAME, FILE_APPEND);
    if (!file)
    {
        return;
    }

    String st_start = isoformat_ctime(&start);
    String st_end = isoformat_ctime(&end);

    file.print(st_start);
    file.print(",");
    file.print(st_end);
    file.print(",");
    file.print(flavor);
    file.print("\n");
    file.close();
}
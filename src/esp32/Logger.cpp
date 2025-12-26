#include <fstream>

#include <esp_log.h>

#include "Logger.h"
#include "SDCard.h"

#include <Arduino.h>

String isoformat_ctime(const time_t* t)
{
    struct tm* timeinfo = localtime(t);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}

void Logger::log_pomodoro(time_t start, time_t end, uint8_t flavor) {
    SDCard with_sd_card;
    if (!with_sd_card)
    {
        return;
    }

    std::ofstream file(FILENAME, std::ios::app);
    if (!file.is_open())
    {
        return;
    }

    String st_start = isoformat_ctime(&start);
    String st_end = isoformat_ctime(&end);

    ESP_LOGI("Logger", "Logging pomodoro: start=%s end=%s flavor=%d", st_start.c_str(), st_end.c_str(), flavor);

    file << st_start.c_str() << "," << st_end.c_str() << "," << static_cast<int>(flavor) << "\n";
}
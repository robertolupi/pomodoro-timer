//
// Created by Roberto Lupi on 01.02.2025.
//

#include "Configuration.h"
#include "Global.h"
#include <SD.h>

ConfigurationClass Configuration;

bool ConfigurationClass::load()
{
    if (!ensureSDMounted())
    {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(spi_mutex);
    File file = SD.open("/config.ini", FILE_READ);
    if (!file)
    {
        return false;
    }

    std::string line;
    std::string section;
    while (file.available())
    {
        String rawLine = file.readStringUntil('\n');
        line = rawLine.c_str();
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (line.empty())
        {
            continue;
        }
        if (line[0] == '[' && line[line.size() - 1] == ']')
        {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        const auto pos = line.find('=');
        if (pos == std::string::npos)
        {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        sections[section][key] = value;
    }
    file.close();
    return true;
}

//
// Created by Roberto Lupi on 26.02.2025.
//

#include "SDCard.h"
#include <SD.h>
#include <M5Unified.h>

std::mutex SDCard::mutex;
bool SDCard::mounted = false;

SDCard::SDCard()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!mounted)
    {
        if (!SD.begin(M5.getPin(m5::sd_spi_ss)))
        {
            Serial.print("SD Card Mount Failed\n");
        } else {
          mounted = true;
        }
    }
}

SDCard::~SDCard()
{
}
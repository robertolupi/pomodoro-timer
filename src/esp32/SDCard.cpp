//
// Created by Roberto Lupi on 26.02.2025.
//

#include "SDCard.h"
#include <SD.h>
#include <M5Unified.h>
#include "SPILock.h"

bool SDCard::mounted = false;
bool SDCard::attempt_made = false;

SDCard::SDCard()
{
    SPILock lock;
    if (!attempt_made)
    {
        attempt_made = true;
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
#include <stdexcept>

#include <Arduino.h>
#include <SD.h> // must be included before M5Unified.h
#include <WiFi.h>

#include <M5Unified.h>
#include <esp_log.h>

#include "WiFiSettings.h"
#include "Configuration.h"
#include "Pomodoro.h"
#include "ClockFace.h"
#include "Splash.h"
#include "Gong.h"
#include "Logger.h"
#include "Leds.h"


void setup()
{
    M5.begin();
    Serial.begin(115200);

    try
    {
        Splash splash;
        std::string ssid = wifi::ssid;
        std::string password = wifi::password;
        std::string ntpServer = "pool.ntp.org";
        std::string timezone = "CET-1CEST,M3.5.0,M10.5.0/3";

        if (Configuration.load()) {
            ssid = Configuration["wifi"]["ssid"];
            if (ssid.empty())
                throw std::runtime_error("WiFi SSID not found");
            password = Configuration["wifi"]["pass"];
            if (password.empty())
                throw std::runtime_error("WiFi password not found");
            ntpServer = Configuration["ntp"]["host"];
            if (ntpServer.empty())
                throw std::runtime_error("NTP server not found");
            timezone = Configuration["ntp"]["tz"];
        }
        WiFi.begin(ssid.c_str(), password.c_str());
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
        }
        configTzTime(timezone.c_str(), ntpServer.c_str());
    }
    catch (const std::exception& e)
    {
        M5.Lcd.setTextSize(2);
        M5.Lcd.print(e.what());
        Serial.println("Exception: " + String(e.what()));
        return;
    }

    PomodoroClock pomodoro;
    Logger logger;
    pomodoro.add_observer(logger);

    ClockFace clock_face;
    Gong gong;
    Leds leds;
    pomodoro.add_observer(clock_face);
    pomodoro.add_observer(gong);
    pomodoro.add_observer(leds);


    while (true)
    {
        bool buttons = M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed();
        if (!buttons) { pomodoro.PassageOfTime(); }
        else
        {
            switch (pomodoro.State())
            {
            case IDLE:
                if (M5.BtnA.wasPressed())
                {
                    pomodoro.StartWork(0);
                } else if (M5.BtnB.wasPressed())
                {
                    pomodoro.StartWork(1);
                } else if (M5.BtnC.wasPressed())
                {
                    pomodoro.StartWork(2);
                }
                break;
            case WORK:
                if (M5.BtnB.wasPressed())
                {
                    pomodoro.ExtendWork();
                }
                if (M5.BtnC.wasPressed())
                {
                    pomodoro.Cancel();
                }
                break;
            case BREAK:
                if (M5.BtnC.wasPressed())
                {
                    pomodoro.Cancel();
                }
                break;
            }
        }

        // wait for the next second
        const time_t now = time(nullptr);
        for (int i = 0; i < 10; i++) {
            M5.update();
            if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed())
            {
                break;
            }
            if (time(nullptr) > now) {
                break;
            }
            delay(100);
        }
    }
}

void loop()
{
}

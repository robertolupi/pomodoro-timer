//
// Created by Roberto Lupi on 06.02.2025.
//

#ifndef CLOCKFACE_H
#define CLOCKFACE_H

#include <array>

#include <M5Unified.h>

#include "Pomodoro.h"

class ClockFace final : public PomodoroObserver
{
public:
    ClockFace()
        : canvas_(&M5.Lcd),
          flavor_labels_({String("0"), String("1"), String("2")})
    {
        canvas_.createSprite(M5.Lcd.width(), M5.Lcd.height());
    }

    ~ClockFace() override
    {
        canvas_.deleteSprite();
    }


    void notification(ClockUpdate update) override;

    void notification(::IdleToWork) override
    {
    }

    void notification(::WorkToBreak) override
    {
    };

    void notification(::BreakToIdle) override
    {
    };

    void notification(::WorkToIdle) override
    {
    };

    void notification(::AdditionalWork) override
    {
    };

    void setFlavorLabels(const std::array<String, 3>& labels)
    {
        flavor_labels_ = labels;
    }

private:
    M5Canvas canvas_;
    std::array<String, 3> flavor_labels_;

    void drawTime(const char* time, int color, int line, int font, int font_size);
    void drawFlavor(const uint8_t flavor);
};


#endif //CLOCKFACE_H

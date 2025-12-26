#ifndef LOGGER_H
#define LOGGER_H

#include <M5Unified.h>
#include "Pomodoro.h"

class Logger : public PomodoroObserver {

    const char* FILENAME = "/sd/pomodoro.csv";

    time_t start_time = 0;
    uint8_t work_flavor = 0;

    void log_pomodoro(time_t start, time_t end, uint8_t flavor);

    public:

    void notification(IdleToWork update) override {
        start_time = update.now;
        work_flavor = update.work_flavor;
    }
    void notification(WorkToBreak update) override {
        log_pomodoro(start_time, update.now, work_flavor);
    }
    void notification(WorkToIdle update) override {
        log_pomodoro(start_time, update.now, work_flavor);
    }
    void notification(AdditionalWork update) override {}
    void notification(BreakToIdle update) override {}
    void notification(ClockUpdate update) override {}
};

#endif //LOGGER_H
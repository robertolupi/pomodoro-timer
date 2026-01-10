//
// Created by Roberto Lupi on 02.02.2025.
//

#include "Pomodoro.h"

#include <cstdlib>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
#endif

bool PomodoroClock::StartWork(const uint8_t flavor, const time_t work_duration, const time_t break_duration, const time_t now)
{
    if (state_ != IDLE)
    {
        return false;
    }
    state_ends_at_ = now + work_duration;
    last_update_at_ = now;
    last_state_change_at_ = now;
    state_ = WORK;
    work_flavor_ = flavor;
    break_duration_ = break_duration;
    const IdleToWork update = {work_flavor_, now};
    notify_observers(update);
    PassageOfTime(now);
    return true;
}

bool PomodoroClock::ExtendWork(const time_t additional_work_duration, const time_t now)
{
    if (state_ != WORK)
    {
        return false;
    }
    state_ends_at_ += additional_work_duration > 0 ? additional_work_duration : break_duration_;
    const AdditionalWork update = {now, work_flavor_, state_ends_at_};
    last_update_at_ = now;
    notify_observers(update);
    PassageOfTime(now);
    return true;
}

bool PomodoroClock::Cancel(const time_t now)
{
    bool result;
    const WorkToIdle work_to_idle = {now, now - last_state_change_at_};
    const BreakToIdle break_to_idle = {now, now - last_state_change_at_};
    switch (state_)
    {
    case WORK:
        state_ = IDLE;
        last_state_change_at_ = now;
        notify_observers(work_to_idle);
        work_flavor_ = 0;
        result = true;
        break;
    case BREAK:
        state_ = IDLE;
        last_state_change_at_ = now;
        notify_observers(break_to_idle);
        work_flavor_ = 0;
        result = true;
        break;
    case IDLE:
    default:
        result = false;
    }
    PassageOfTime(now);
    return result;
}

void PomodoroClock::PassageOfTime(const time_t now)
{
    bool state_change = (state_ends_at_ != 0) && (now >= state_ends_at_);
    const WorkToBreak work_to_break = {now, state_ends_at_ - last_state_change_at_};
    const BreakToIdle break_to_idle = {now, state_ends_at_ - last_state_change_at_};
    if (state_change)
    {
        switch (state_)
        {
        case WORK:
            state_ = BREAK;
            last_state_change_at_ = now;
            state_ends_at_ = state_ends_at_ + break_duration_;
            notify_observers(work_to_break);
            break;
        case BREAK:
            state_ = IDLE;
            last_state_change_at_ = state_ends_at_;
            state_ends_at_ = 0;
            notify_observers(break_to_idle);
            work_flavor_ = 0;
            break;
        default:
            break;
        }
    }
    last_update_at_ = now;
    ClockUpdate update = {now, state_, work_flavor_, state_ends_at_ - now};
    notify_observers(update);
}

PomodoroWatchdog::PomodoroWatchdog(const time_t timeout_seconds)
    : timeout_seconds_(timeout_seconds),
      last_update_(0)
#if defined(ARDUINO_ARCH_ESP32)
    , task_(nullptr)
#endif
{
#if defined(ARDUINO_ARCH_ESP32)
    xTaskCreatePinnedToCore(taskTrampoline, "PomodoroWatchdog", 2048, this, 1, &task_, 0);
#endif
}

void PomodoroWatchdog::notification(const ClockUpdate update)
{
    touch(update.now);
}

void PomodoroWatchdog::notification(const IdleToWork update)
{
    touch(update.now);
}

void PomodoroWatchdog::notification(const WorkToBreak update)
{
    touch(update.now);
}

void PomodoroWatchdog::notification(const BreakToIdle update)
{
    touch(update.now);
}

void PomodoroWatchdog::notification(const WorkToIdle update)
{
    touch(update.now);
}

void PomodoroWatchdog::notification(const AdditionalWork update)
{
    touch(update.now);
}

void PomodoroWatchdog::touch(const time_t now)
{
    last_update_ = now;
    check(now);
}

void PomodoroWatchdog::check(const time_t now)
{
    if (timeout_seconds_ <= 0)
    {
        return;
    }
    const time_t last = last_update_;
    if (last == 0)
    {
        return;
    }
    if (now - last > timeout_seconds_)
    {
#if defined(ARDUINO_ARCH_ESP32)
        esp_restart();
#else
        std::abort();
#endif
    }
}

#if defined(ARDUINO_ARCH_ESP32)
void PomodoroWatchdog::taskTrampoline(void* context)
{
    PomodoroWatchdog* self = static_cast<PomodoroWatchdog*>(context);
    if (self)
    {
        self->taskLoop();
    }
    vTaskDelete(nullptr);
}

void PomodoroWatchdog::taskLoop()
{
    const TickType_t delay_ticks = pdMS_TO_TICKS(1000);
    for (;;)
    {
        check(time(nullptr));
        vTaskDelay(delay_ticks);
    }
}
#endif

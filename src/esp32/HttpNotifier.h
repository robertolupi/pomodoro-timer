//
// Created by Roberto Lupi on 27.12.2025.
//

#ifndef HTTPNOTIFIER_H
#define HTTPNOTIFIER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "Pomodoro.h"

class HttpNotifier final : public PomodoroObserver
{
public:
    HttpNotifier(const char* host, uint16_t port);

    void notification(ClockUpdate update) override;
    void notification(IdleToWork update) override;
    void notification(WorkToBreak update) override;
    void notification(BreakToIdle update) override;
    void notification(WorkToIdle update) override;
    void notification(AdditionalWork) override {}

private:
    String host_;
    uint16_t port_;
    time_t current_start_time_;
    bool enabled_;
    TaskHandle_t queue_task_;
    SemaphoreHandle_t sd_mutex_;

    bool ensureQueueDir();
    String makeQueueFilename(time_t event_time);
    String makePayload(time_t event_time, time_t start_time, const char* transition, const String& extra_json);
    bool enqueueEvent(time_t event_time, time_t start_time, const char* transition, const String& extra_json);
    bool flushQueueOnce();
    bool sendPayload(const String& payload, time_t start_time);
    bool extractUInt64(const String& payload, const char* key, unsigned long long* value) const;
    void notifyQueueTask();
    bool lockSd(TickType_t timeout);
    void unlockSd();
    static void queueTaskTrampoline(void* context);
    void queueTask();
};

#endif //HTTPNOTIFIER_H

//
// Created by Roberto Lupi on 27.12.2025.
//

#include "HttpNotifier.h"

#include <ctype.h>

#include <HTTPClient.h>
#include <SD.h>
#include <WiFi.h>

#include "SDCard.h"

HttpNotifier::HttpNotifier(const char* host, const uint16_t port)
    : host_(host ? host : ""),
      port_(port),
      current_start_time_(0),
      current_work_flavor_(0),
      enabled_(false),
      queue_task_(nullptr),
      sd_mutex_(nullptr),
      flavor_labels_({String("0"), String("1"), String("2")})
{
    enabled_ = host_.length() > 0 && port_ > 0;
    if (enabled_)
    {
        sd_mutex_ = xSemaphoreCreateMutex();
        SDCard with_sd_card;
        if (with_sd_card)
        {
            if (lockSd(pdMS_TO_TICKS(250)))
            {
                ensureQueueDir();
                unlockSd();
            }
        }
        xTaskCreatePinnedToCore(queueTaskTrampoline, "HttpNotifyQueue", 8192, this, 1, &queue_task_, 0);
        notifyQueueTask();
    }
}

void HttpNotifier::setFlavorLabels(const std::array<String, 3>& labels)
{
    flavor_labels_ = labels;
}

void HttpNotifier::notification(const ClockUpdate)
{
    if (!enabled_)
    {
        return;
    }
    notifyQueueTask();
}

void HttpNotifier::notification(const IdleToWork update)
{
    if (!enabled_)
    {
        return;
    }
    current_start_time_ = update.now;
    current_work_flavor_ = update.work_flavor;
    const String extra_json = String("\"work_flavor\":\"") + escapeJsonString(flavorLabel(update.work_flavor)) + "\"";
    enqueueEvent(update.now, current_start_time_, "idle_to_work", extra_json);
    notifyQueueTask();
}

void HttpNotifier::notification(const WorkToBreak update)
{
    if (!enabled_)
    {
        return;
    }
    const time_t start_time = current_start_time_ > 0 ? current_start_time_ : update.now - update.work_duration;
    current_start_time_ = start_time;
    const String extra_json = String("\"work_duration\":")
        + String(static_cast<unsigned long>(update.work_duration))
        + ",\"work_flavor\":\""
        + escapeJsonString(flavorLabel(current_work_flavor_))
        + "\"";
    enqueueEvent(update.now, start_time, "work_to_break", extra_json);
    notifyQueueTask();
}

void HttpNotifier::notification(const BreakToIdle update)
{
    if (!enabled_)
    {
        return;
    }
    const time_t start_time = current_start_time_ > 0 ? current_start_time_ : update.now;
    const String extra_json = String("\"break_duration\":") + String(static_cast<unsigned long>(update.break_duration));
    enqueueEvent(update.now, start_time, "break_to_idle", extra_json);
    current_start_time_ = 0;
    current_work_flavor_ = 0;
    notifyQueueTask();
}

void HttpNotifier::notification(const WorkToIdle update)
{
    if (!enabled_)
    {
        return;
    }
    const time_t start_time = current_start_time_ > 0 ? current_start_time_ : update.now - update.cancelled_work_duration;
    const String extra_json = String("\"cancelled_work_duration\":")
        + String(static_cast<unsigned long>(update.cancelled_work_duration))
        + ",\"work_flavor\":\""
        + escapeJsonString(flavorLabel(current_work_flavor_))
        + "\"";
    enqueueEvent(update.now, start_time, "work_to_idle", extra_json);
    current_start_time_ = 0;
    current_work_flavor_ = 0;
    notifyQueueTask();
}

bool HttpNotifier::ensureQueueDir()
{
    if (SD.exists("/queue"))
    {
        return true;
    }
    return SD.mkdir("/queue");
}

String HttpNotifier::makeQueueFilename(const time_t event_time)
{
    const unsigned long long timestamp = static_cast<unsigned long long>(event_time);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%010llu.json", timestamp);
    String path = String("/queue/") + buffer;
    if (!SD.exists(path))
    {
        return path;
    }
    for (unsigned int i = 1; i < 1000; i++)
    {
        snprintf(buffer, sizeof(buffer), "%010llu-%u.json", timestamp, i);
        path = String("/queue/") + buffer;
        if (!SD.exists(path))
        {
            return path;
        }
    }
    return path;
}

String HttpNotifier::makePayload(const time_t event_time, const time_t start_time, const char* transition, const String& extra_json)
{
    String payload = "{";
    payload += "\"transition\":\"";
    payload += transition;
    payload += "\",\"start_time\":";
    payload += String(static_cast<unsigned long>(start_time));
    payload += ",\"event_time\":";
    payload += String(static_cast<unsigned long>(event_time));
    if (extra_json.length() > 0)
    {
        payload += ",";
        payload += extra_json;
    }
    payload += "}";
    return payload;
}

bool HttpNotifier::enqueueEvent(const time_t event_time, const time_t start_time, const char* transition, const String& extra_json)
{
    SDCard with_sd_card;
    if (!with_sd_card)
    {
        return false;
    }
    if (!lockSd(pdMS_TO_TICKS(200)))
    {
        return false;
    }
    if (!ensureQueueDir())
    {
        unlockSd();
        return false;
    }

    const String path = makeQueueFilename(event_time);
    const String payload = makePayload(event_time, start_time, transition, extra_json);
    File file = SD.open(path, FILE_WRITE);
    if (!file)
    {
        unlockSd();
        return false;
    }
    file.print(payload);
    file.close();
    unlockSd();
    return true;
}

bool HttpNotifier::extractUInt64(const String& payload, const char* key, unsigned long long* value) const
{
    String token = String("\"") + key + "\":";
    const int start = payload.indexOf(token);
    if (start < 0)
    {
        return false;
    }
    int index = start + token.length();
    while (index < payload.length() && payload[index] == ' ')
    {
        index++;
    }
    int end = index;
    while (end < payload.length() && isdigit(static_cast<unsigned char>(payload[end])))
    {
        end++;
    }
    if (end == index)
    {
        return false;
    }
    *value = strtoull(payload.substring(index, end).c_str(), nullptr, 10);
    return true;
}

String HttpNotifier::flavorLabel(const uint8_t flavor) const
{
    if (flavor < flavor_labels_.size() && flavor_labels_[flavor].length() > 0)
    {
        return flavor_labels_[flavor];
    }
    return String(flavor);
}

String HttpNotifier::escapeJsonString(const String& input) const
{
    String output;
    output.reserve(input.length());
    for (unsigned int i = 0; i < input.length(); i++)
    {
        const char ch = input[i];
        switch (ch)
        {
        case '\\':
        case '"':
            output += '\\';
            output += ch;
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            output += ch;
            break;
        }
    }
    return output;
}

bool HttpNotifier::flushQueueOnce()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    SDCard with_sd_card;
    if (!with_sd_card)
    {
        return false;
    }

    String path;
    String payload;
    if (!lockSd(portMAX_DELAY))
    {
        return false;
    }
    File dir = SD.open("/queue");
    if (!dir || !dir.isDirectory())
    {
        unlockSd();
        return false;
    }

    time_t oldest_time = 0;
    String oldest_name;
    File entry = dir.openNextFile();
    while (entry)
    {
        if (!entry.isDirectory())
        {
            const String name = entry.name();
            char* endptr = nullptr;
            const unsigned long long ts = strtoull(name.c_str(), &endptr, 10);
            if (ts > 0)
            {
                if (oldest_time == 0 || ts < static_cast<unsigned long long>(oldest_time)
                    || (ts == static_cast<unsigned long long>(oldest_time) && name < oldest_name))
                {
                    oldest_time = static_cast<time_t>(ts);
                    oldest_name = name;
                }
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    if (oldest_name.length() == 0)
    {
        unlockSd();
        return false;
    }

    path = String("/queue/") + oldest_name;
    File file = SD.open(path, FILE_READ);
    if (!file)
    {
        unlockSd();
        return false;
    }
    while (file.available())
    {
        payload += static_cast<char>(file.read());
    }
    file.close();
    unlockSd();

    unsigned long long start_time = 0;
    if (!extractUInt64(payload, "start_time", &start_time))
    {
        return false;
    }

    if (!sendPayload(payload, static_cast<time_t>(start_time)))
    {
        return false;
    }

    if (lockSd(portMAX_DELAY))
    {
        SD.remove(path);
        unlockSd();
    }
    return true;
}

bool HttpNotifier::sendPayload(const String& payload, const time_t start_time)
{
    if (!enabled_)
    {
        return false;
    }

    HTTPClient http;
    WiFiClient client;
    String url = "http://" + host_ + ":" + String(port_) + "/pomodoros/" + String(static_cast<unsigned long>(start_time)) + "/transitions";
    if (!http.begin(client, url))
    {
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000);
    const int code = http.POST(payload);
    http.end();
    return code >= 200 && code < 300;
}

void HttpNotifier::notifyQueueTask()
{
    if (queue_task_)
    {
        xTaskNotifyGive(queue_task_);
    }
}

bool HttpNotifier::lockSd(const TickType_t timeout)
{
    if (!sd_mutex_)
    {
        return true;
    }
    return xSemaphoreTake(sd_mutex_, timeout) == pdTRUE;
}

void HttpNotifier::unlockSd()
{
    if (sd_mutex_)
    {
        xSemaphoreGive(sd_mutex_);
    }
}

void HttpNotifier::queueTaskTrampoline(void* context)
{
    HttpNotifier* self = static_cast<HttpNotifier*>(context);
    if (self)
    {
        self->queueTask();
    }
    vTaskDelete(nullptr);
}

void HttpNotifier::queueTask()
{
    const TickType_t wait_ticks = pdMS_TO_TICKS(5000);
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, wait_ticks);
        if (!enabled_)
        {
            continue;
        }
        while (flushQueueOnce())
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

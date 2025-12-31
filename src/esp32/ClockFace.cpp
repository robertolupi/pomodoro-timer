//
// Created by Roberto Lupi on 06.02.2025.
//

#include "ClockFace.h"

const char* days_of_week[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

int weekday(const time_t t)
{
    struct tm *timeinfo = localtime(&t);
    return timeinfo->tm_wday;
}

inline void ClockFace::notification(ClockUpdate update)
{
  char time_buffer[sizeof("HH:MM:SS")];
  char time_buffer2[sizeof("DD MM YYYY")];
  strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", localtime(&update.now));
  strftime(time_buffer2, sizeof(time_buffer2), "%d %m %Y", localtime(&update.now));
  int weekday_index = weekday(update.now) % 7;
  char remaining_time_buffer[sizeof("MM:SS")];
  strftime(remaining_time_buffer, sizeof(remaining_time_buffer), "%M:%S", gmtime(&update.remaining_time_in_state));
  switch(update.state) {
  case IDLE:
    canvas_.fillScreen(BLACK);
    drawTime(time_buffer, WHITE, 0, 7, 1);
    drawTime(time_buffer2, YELLOW, 1, 4, 1);
    drawTime(days_of_week[weekday_index], YELLOW, 2, 4, 1);
    break;
  case WORK:
    canvas_.fillScreen(DARKGREEN);
    drawFlavor(update.work_flavor);
    drawTime(time_buffer, WHITE, 0, 7, 1);
    drawTime(remaining_time_buffer, BLACK, 1, 7, 1);
    break;
  case BREAK:
    canvas_.fillScreen(RED);
    drawTime(time_buffer, WHITE, 0, 7, 1);
    drawTime(remaining_time_buffer, BLACK, 1, 7, 1);
    break;
  }

  canvas_.pushSprite(0, 0);
}

void ClockFace::drawTime(const char* time, int color, int line, int font, int font_size)
{
  canvas_.setTextColor(color);
  canvas_.setTextSize(font_size);
  canvas_.setTextFont(font);
  const int padding = canvas_.fontHeight() / 4;
  canvas_.drawCenterString(time, canvas_.width() / 2, canvas_.height() / 2 - (canvas_.fontHeight() + padding) + (canvas_.fontHeight() + padding) * line);
}


void ClockFace::drawFlavor(const uint8_t flavor)
{
  canvas_.setTextColor(BLACK);
  canvas_.setTextSize(1);
  canvas_.setTextFont(4);
  String label = String(flavor);
  if (flavor < flavor_labels_.size() && flavor_labels_[flavor].length() > 0)
  {
    label = flavor_labels_[flavor];
  }
  canvas_.drawString(label, 10, 10);
}

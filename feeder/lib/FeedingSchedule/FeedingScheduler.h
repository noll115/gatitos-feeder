#ifndef FEEDING_SCHEDULER_H
#define FEEDING_SCHEDULER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <StreamUtils.h>

#define FEEDING_PORTION_COUNT 3
#define FEEDING_SCHEDULE_SIZE sizeof(FeedingTime) * FEEDING_PORTION_COUNT
#define FEEDING_SCHEDULE_SIZE_JSON 256
#define FEEDING_SCHEDULE_NEVER_RAN 255
struct ScheduledFeeding {
  uint8_t hour;    // 24hr
  uint8_t minute;  // 60 mins
  uint8_t portion;
  uint8_t lastDayRan = FEEDING_SCHEDULE_NEVER_RAN;  // 0-6, 255 = never ran
};

class FeedingScheduler {
 private:
  ScheduledFeeding feedingSchedule[FEEDING_PORTION_COUNT];
  ScheduledFeeding* currentFeeding = nullptr;
  WiFiUDP ntpUDP;
  NTPClient ntpClient;
  size_t currentDocSize;

 public:
  FeedingScheduler();
  void begin();
  bool updateSchedules(const byte* payload, uint length);
  bool isFeedingTime();
  void getFeedingTimesStr(byte* buffer);
  ScheduledFeeding* getCurrentFeedingTime();
  size_t getCurrentDocSize();
  void loop();
};

#endif  // FEEDING_SCHEDULER_H

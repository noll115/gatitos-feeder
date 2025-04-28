#ifndef FEEDING_SCHEDULER_H
#define FEEDING_SCHEDULER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define FEEDING_PORTION_COUNT 3
#define FEEDING_SCHEDULE_SIZE sizeof(FeedingTime) * FEEDING_PORTION_COUNT
#define FEEDING_SCHEDULE_SIZE_JSON 256

struct FeedingTime {
  uint8_t hour;    // 24hr
  uint8_t minute;  // 60 mins
  uint8_t portion;
};

class FeedingScheduler {
 private:
  FeedingTime feedingTimes[FEEDING_PORTION_COUNT];
  uint8_t feedingTimesIndex;
  WiFiUDP ntpUDP;
  NTPClient ntpClient;

 public:
  FeedingScheduler();
  void begin();
  bool updateSchedules(const byte* payload, uint length);
  bool isFeedingTime();
  JsonDocument getFeedingTimes();
  FeedingTime& getCurrentFeedingTime();
  void loop();
};

#endif  // FEEDING_SCHEDULER_H

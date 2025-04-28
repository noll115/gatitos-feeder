
#include "FeedingScheduler.h"
#include <StreamUtils.h>
#include <EEPROM.h>

FeedingScheduler::FeedingScheduler() : ntpClient(ntpUDP, "pool.ntp.org") {
  EEPROM.begin(FEEDING_SCHEDULE_SIZE_JSON);
  Serial.print("Scheduler initialized");
  // Load schedules from EEPROM
  EepromStream stream(0, FEEDING_SCHEDULE_SIZE_JSON);
  JsonDocument jsonFeedingTimes;
  DeserializationError error = deserializeJson(jsonFeedingTimes, stream);
  if (error) {
    Serial.print(F("Failed to read schedules from EEPROM: "));
    Serial.println(error.c_str());
    return;
  }
  JsonArray array = jsonFeedingTimes.as<JsonArray>();
  for (size_t i = 0; i < FEEDING_PORTION_COUNT; i++) {
    JsonObject obj = array[i].as<JsonObject>();
    feedingTimes[i].hour = obj["hour"];
    feedingTimes[i].minute = obj["minute"];
    feedingTimes[i].portion = obj["portion"];
  }
}

void FeedingScheduler::begin() {
  Serial.println("Starting NTP client...");
  ntpClient.begin();
}

bool FeedingScheduler::updateSchedules(const byte* payload, uint length) {
  JsonDocument doc;
  deserializeJson(doc, payload);
  Serial.println("Updating schedules...");
  serializeJsonPretty(doc, Serial);
  EepromStream stream(0, FEEDING_SCHEDULE_SIZE_JSON);
  serializeJson(doc, stream);
  stream.flush();
  return true;
}

bool FeedingScheduler::isFeedingTime() {
  int currentHour = ntpClient.getHours();
  int currentMinute = ntpClient.getMinutes();

  for (size_t i = 0; i < FEEDING_PORTION_COUNT; i++) {
    if (feedingTimes[i].hour == currentHour &&
        feedingTimes[i].minute == currentMinute) {
      feedingTimesIndex = i;
      return true;
    }
  }
  return false;
}

JsonDocument FeedingScheduler::getFeedingTimes() {
  JsonDocument doc;
  for (size_t i = 0; i < FEEDING_PORTION_COUNT; i++) {
    JsonObject obj = doc.add<JsonObject>();
    obj["hour"] = feedingTimes[i].hour;
    obj["minute"] = feedingTimes[i].minute;
    obj["portion"] = feedingTimes[i].portion;
  }
  return doc;
}

FeedingTime& FeedingScheduler::getCurrentFeedingTime() {
  return feedingTimes[this->feedingTimesIndex];
}

void FeedingScheduler::loop() { ntpClient.update(); }

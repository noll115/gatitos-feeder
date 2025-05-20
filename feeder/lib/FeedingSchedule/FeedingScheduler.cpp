
#include "FeedingScheduler.h"

FeedingScheduler::FeedingScheduler() : ntpClient(ntpUDP, "pool.ntp.org") {}

void FeedingScheduler::begin() {
  // Serial.println("Scheduler initialized");
  // Load schedules from EEPROM
  EepromStream stream(0, FEEDING_SCHEDULE_SIZE_JSON);
  JsonDocument jsonFeedingTimes;

  DeserializationError error = deserializeJson(jsonFeedingTimes, stream);
  if (error || jsonFeedingTimes.isNull()) {
    // Serial.print(F("Failed to read schedules from EEPROM: "));
    // Serial.println(error.c_str());
    // Default schedules
    JsonArray array = jsonFeedingTimes.to<JsonArray>();
    JsonObject obj = array[0].to<JsonObject>();
    obj["hour"] = 8;
    obj["minute"] = 0;
    obj["portion"] = 2;
    obj["lastDayRan"] = FEEDING_SCHEDULE_NEVER_RAN;
    obj = array[1].to<JsonObject>();
    obj["hour"] = 12;
    obj["minute"] = 0;
    obj["portion"] = 2;
    obj["lastDayRan"] = FEEDING_SCHEDULE_NEVER_RAN;
    obj = array[2].to<JsonObject>();
    obj["hour"] = 18;
    obj["minute"] = 0;
    obj["portion"] = 2;
    obj["lastDayRan"] = FEEDING_SCHEDULE_NEVER_RAN;

    this->currentDocSize = measureJson(jsonFeedingTimes);
    // Serial.println(jsonFeedingTimes.as<String>());
    EepromStream stream(0, this->currentDocSize);
    serializeJson(jsonFeedingTimes, stream);
    stream.flush();
    // Serial.println("Default schedules written to EEPROM");
  } else {
    this->currentDocSize = measureJson(jsonFeedingTimes);
  }
  JsonArray array = jsonFeedingTimes.as<JsonArray>();
  for (size_t i = 0; i < FEEDING_PORTION_COUNT; i++) {
    JsonObject obj = array[i].as<JsonObject>();
    feedingSchedule[i].hour = obj["hour"];
    feedingSchedule[i].minute = obj["minute"];
    feedingSchedule[i].portion = obj["portion"];
    feedingSchedule[i].lastDayRan =
        obj["lastDayRan"] | FEEDING_SCHEDULE_NEVER_RAN;
  }
  Serial.println("Starting NTP client...");
  ntpClient.begin();
}

bool FeedingScheduler::updateSchedules(const byte* payload, uint length) {
  JsonDocument doc;
  deserializeJson(doc, payload);
  // Serial.println("Updating schedules...");
  serializeJsonPretty(doc, Serial);
  this->currentDocSize = measureJson(doc);
  EepromStream stream(0, this->currentDocSize);
  serializeJson(doc, stream);
  stream.flush();
  EEPROM.write(this->currentDocSize, '\0');

  for (size_t i = 0; i < FEEDING_PORTION_COUNT; i++) {
    JsonObject obj = doc[i].as<JsonObject>();
    feedingSchedule[i].hour = obj["hour"];
    feedingSchedule[i].minute = obj["minute"];
    feedingSchedule[i].portion = obj["portion"];
    feedingSchedule[i].lastDayRan = FEEDING_SCHEDULE_NEVER_RAN;
  }
  return true;
}

bool FeedingScheduler::isFeedingTime() {
  int currentHour = ntpClient.getHours();
  int currentMinute = ntpClient.getMinutes();
  int currentDay = ntpClient.getDay();

  for (size_t i = 0; i < FEEDING_PORTION_COUNT; i++) {
    bool isNotSameDay =
        (feedingSchedule[i].lastDayRan == FEEDING_SCHEDULE_NEVER_RAN ||
         feedingSchedule[i].lastDayRan != currentDay);

    if (isNotSameDay && feedingSchedule[i].hour == currentHour &&
        feedingSchedule[i].minute == currentMinute) {
      feedingSchedule[i].lastDayRan = currentDay;
      this->currentFeeding = feedingSchedule + i;
      return true;
    }
  }
  return false;
}

void FeedingScheduler::getFeedingTimesStr(byte* buffer) {
  EepromStream stream(0, FEEDING_SCHEDULE_SIZE_JSON);
  stream.readBytes(buffer, FEEDING_SCHEDULE_SIZE_JSON);
  stream.flush();
}

ScheduledFeeding* FeedingScheduler::getCurrentFeedingTime() {
  return this->currentFeeding;
}

void FeedingScheduler::loop() { ntpClient.update(); }

size_t FeedingScheduler::getCurrentDocSize() { return currentDocSize; }

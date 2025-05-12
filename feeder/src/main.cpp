#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Arduino.h>
#include <ezButton.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ElegantOTA.h>
#include "FeedingScheduler.h"

// JS50T 5V
// 0.07A seen 0.1A
#define DEBUG 1
#if DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(x, ...)
#endif
#define ID_NAME(name) #name

const char* ssid = "LOKI";
const char* password = "curlymesa627";
const char* serverUrl = "feeders.gatitos.cloud";

// max length is 20
const char* deviceId = "loki";

// topics to subscribe
char setFeedingSchedule[50];
char getFeedingSchedule[50];
char commandTopic[50];
// topics to publish
char feedingTimesTopic[50];
char stateChangeTopic[50];
char ipAddressTopic[50];

ezButton button(D1,
                INPUT_PULLUP);  // create ezButton object that attach to pin D1
uint8_t MOTOR_PIN = D5;         // define motor pin
uint8_t portionsDispensed = 0;
bool feedingOnce = false;
enum STATES : uint8_t { IDLE, START, UNLOCKING, ROTATING, LOCK };

STATES state = IDLE;

WiFiClient espClient;
PubSubClient mqttClient(serverUrl, 1883, espClient);
ESP8266WebServer server(80);
FeedingScheduler scheduler;

WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;
WiFiEventHandler stationGotIPHandler;

void setupTopics() {
  snprintf(setFeedingSchedule, sizeof(setFeedingSchedule),
           "devices/%s/setFeedingSchedule", deviceId);
  snprintf(getFeedingSchedule, sizeof(getFeedingSchedule),
           "devices/%s/getFeedingSchedule", deviceId);
  snprintf(commandTopic, sizeof(commandTopic), "devices/%s/command", deviceId);
  snprintf(feedingTimesTopic, sizeof(feedingTimesTopic),
           "devices/%s/feedingSchedule", deviceId);
  snprintf(stateChangeTopic, sizeof(stateChangeTopic), "devices/%s/stateChange",
           deviceId);
  snprintf(ipAddressTopic, sizeof(ipAddressTopic), "devices/%s/ipAddress",
           deviceId);
}

void log(const String& message) {
  if (!WiFi.isConnected()) return;
  DEBUG_PRINTLN("Logging message...");
  DEBUG_PRINTLN(message);
  WiFiClient httpClient;
  HTTPClient http;
  http.begin(httpClient, "http://192.168.50.225:3000/api/logs");
  http.addHeader("Content-Type", "application/json");
  JsonDocument doc;
  doc["id"] = deviceId;
  doc["message"] = message;
  String json;
  serializeJson(doc, json);
  int httpResponseCode = http.POST(json);

#if DEBUG
  // Print the response
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
  }
#endif
  http.end();
}

void startFeeding(bool feedOnce) {
  DEBUG_PRINTLN("Starting feeding...");
  state = START;
  feedingOnce = feedOnce;
  portionsDispensed = 0;
  log(String("Feeding started"));
}

void callback(char* topic, byte* payload, unsigned int length) {
  DEBUG_PRINT("Message arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINTLN("] ");

  if (strcmp(topic, commandTopic) == 0) {
    DEBUG_PRINTLN("Command received");
    DEBUG_PRINT("Payload: ");
    payload[length] = '\0';  // Null-terminate the payload
    DEBUG_PRINTLN((char*)payload);
    if (strcmp((char*)payload, "feed") == 0) {
      if (state != IDLE) {
        return;
      }
      startFeeding(true);
    }
  } else if (strcmp(topic, setFeedingSchedule) == 0) {
    scheduler.updateSchedules(payload, length);
  } else if (strcmp(topic, getFeedingSchedule) == 0) {
    byte buffer[FEEDING_SCHEDULE_SIZE_JSON];
    scheduler.getFeedingTimesStr(buffer);
    DEBUG_PRINT("Feeding schedule: ");
    DEBUG_PRINTLN((char*)buffer);
    size_t currentDocSize = scheduler.getCurrentDocSize();
    mqttClient.publish(feedingTimesTopic, buffer, currentDocSize, false);
  }
}

void publishState() {
  if (!mqttClient.connected()) return;
  const char* stateNames[] = {
      ID_NAME(IDLE),     ID_NAME(START), ID_NAME(UNLOCKING),
      ID_NAME(ROTATING), ID_NAME(LOCK),
  };
  String stateString = stateNames[state];
  DEBUG_PRINT("State is: ");
  DEBUG_PRINTLN(stateString);
  mqttClient.publish(stateChangeTopic, stateString.c_str(), true);
  String logMessage = String("State changed: ") + stateString;
  log(logMessage);
}

unsigned long retryInterval = 10000;
unsigned long lastReconnectAttemptMQTT = 0;

void tryConnectMQTT() {
  if (mqttClient.connected() || !WiFi.isConnected()) return;

  unsigned long currentMillis = millis();
  if (currentMillis - lastReconnectAttemptMQTT >= retryInterval) {
    lastReconnectAttemptMQTT = currentMillis;
  } else {
    return;
  }
  if (mqttClient.connect(deviceId)) {
    DEBUG_PRINTLN("Connected to MQTT broker");
    mqttClient.setCallback(callback);
    publishState();
    mqttClient.publish(ipAddressTopic, WiFi.localIP().toString().c_str(), true);
    mqttClient.subscribe(commandTopic);
    mqttClient.subscribe(setFeedingSchedule);
    mqttClient.subscribe(getFeedingSchedule);

  } else {
    DEBUG_PRINT("Failed to connect to MQTT broker, rc=");
    DEBUG_PRINT(mqttClient.state());
    DEBUG_PRINTLN(" try again in 10 seconds");
  }
}

void onStationConnected(const WiFiEventStationModeConnected& event) {
  DEBUG_PRINTLN("Connected to WiFi");
}

void onStationGotIP(
    const WiFiEventStationModeGotIP& event) {  // IP address assigned to ESP
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

void onStationDisconnected(const WiFiEventStationModeDisconnected& event) {
  DEBUG_PRINTLN("Disconnected from WiFi");
}

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  DEBUG_PRINTLN("OTA update started!");
  log("OTA update started");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    DEBUG_PRINTF("OTA Progress Current: %u bytes, Final: %u bytes\n", current,
                 final);
  }
}

void onOTAEnd(bool success) {
  if (success) {
    DEBUG_PRINTLN("OTA update finished successfully!");
    log("OTA update finished successfully!");

  } else {
    DEBUG_PRINTLN("There was an error during OTA update!");
    log("There was an error during OTA update!");
  }
}

void setupWifi() {
  String hostname = String(deviceId) + "-feeder";
  WiFi.hostname(hostname.c_str());
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // DEBUG_PRINT("Connecting to WiFi");
  stationGotIPHandler = WiFi.onStationModeGotIP(&onStationGotIP);
  stationConnectedHandler = WiFi.onStationModeConnected(&onStationConnected);
  stationDisconnectedHandler =
      WiFi.onStationModeDisconnected(&onStationDisconnected);
}

void setupOTAUpdates() {
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
}

void changeState(STATES newState) {
  state = newState;
  publishState();
}

void setup() {
#if DEBUG
  Serial.begin(115200);
#endif
  button.setDebounceTime(100);
  setupTopics();
  EEPROM.begin(FEEDING_SCHEDULE_SIZE_JSON);
  // Check if the state is valid
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  analogWriteFreq(50000);  // Set PWM frequency to 50kHz
  setupWifi();
  setupOTAUpdates();
  scheduler.begin();
  button.loop();
  if (!button.getState() == LOW) {
    changeState(ROTATING);
    // probably stopped mid feeding
    feedingOnce = true;
  }
}

void loop() {
  button.loop();
  mqttClient.loop();
  scheduler.loop();
  server.handleClient();
  ElegantOTA.loop();
  // low = open, high = closed
  int switchState = !button.getState();

  if (state == START) {
    digitalWrite(MOTOR_PIN, HIGH);
    changeState(UNLOCKING);
  } else if (state == UNLOCKING) {
    if (switchState == LOW) {
      changeState(ROTATING);
    }
  } else if (state == ROTATING) {
    digitalWrite(MOTOR_PIN, HIGH);
    if (switchState == HIGH) {
      portionsDispensed++;
      if (!feedingOnce) {
        auto currentPortion = scheduler.getCurrentFeedingTime()->portion;
        if (portionsDispensed < currentPortion) {
          log(String("Portion dispensed: ") + String(portionsDispensed));
          changeState(UNLOCKING);
        } else {
          log(String("Feeding finished ") +
              String(scheduler.getCurrentFeedingTime()->hour) + ":" +
              String(scheduler.getCurrentFeedingTime()->minute) + " " +
              String(currentPortion));
          changeState(LOCK);
        }
      } else {
        log(String("Feeding Once finished"));
        changeState(LOCK);
      }
    }
  } else if (state == LOCK) {
    digitalWrite(MOTOR_PIN, LOW);
    changeState(IDLE);
  } else if (state == IDLE) {
    if (scheduler.isFeedingTime()) {
      DEBUG_PRINTLN("Feeding time!");
      auto currentPortion = scheduler.getCurrentFeedingTime()->portion;
      if (currentPortion > 0) {
        startFeeding(false);
      }
    }
    if (Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      DEBUG_PRINTLN(command);
      if (command.length() > 0 && command[0] == 'u') {
        startFeeding(false);
      } else if (command.length() > 0 && command[0] == 'f') {
        startFeeding(true);
      }
    }
  }

  tryConnectMQTT();
}
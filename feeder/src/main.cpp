#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ezButton.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "FeedingScheduler.h"

// JS50T 5V
// 0.07A seen 0.1A

const char* ssid = "LOKI";
const char* password = "curlymesa627";
const char* mqttServer = "gatitos.cloud/mqtt";

const char* deviceId = "loki";
// MQTT topics
const char* updateSchedules = "devices/loki/updateSchedules";
const char* statusTopic = "devices/loki/status";
const char* commandTopic = "devices/loki/command";

ezButton button(D1,
                INPUT_PULLUP);  // create ezButton object that attach to pin D1
uint8_t MOTOR_PIN = D5;         // define motor pin
bool unlockMotor = false;
uint8_t rotateCount = 0;
bool feedingOnce = false;
enum STATES { IDLE, START, UNLOCKING, ROTATING, LOCK };
STATES state = IDLE;

WiFiClient espClient;
PubSubClient mqttClient(mqttServer, 1883, espClient, Serial);
FeedingScheduler scheduler;

void startFeeding(bool feedOnce) {
  Serial.println("Starting feeding...");
  state = START;
  feedingOnce = feedOnce;
  rotateCount = 1;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  if (strcmp(topic, commandTopic) == 0) {
    if (payload[0] == 'u') {
      startFeeding(false);
    }
  } else if (strcmp(topic, updateSchedules) == 0) {
    scheduler.updateSchedules(payload, length);
  } else if (strcmp(topic, statusTopic) == 0) {
    Serial.println("Status topic");
    // Handle status topic
    // You can send the current feeding times or any other status information
    auto feedingTimes = scheduler.getFeedingTimes();
    char buffer[FEEDING_SCHEDULE_SIZE_JSON];
    size_t size =
        serializeJson(feedingTimes, buffer, FEEDING_SCHEDULE_SIZE_JSON);
    mqttClient.publish(statusTopic, buffer, size);
  }
}

unsigned long retryInterval = 10000;
unsigned long lastReconnectAttempt = 0;

void tryConnectMQTT() {
  if (mqttClient.connected()) return;
  unsigned long currentMillis = millis();
  if (currentMillis - lastReconnectAttempt >= retryInterval) {
    lastReconnectAttempt = currentMillis;
  } else {
    return;
  }
  if (mqttClient.connect(deviceId)) {
    Serial.println("Connected to MQTT broker");
    mqttClient.subscribe(commandTopic);
    mqttClient.subscribe(updateSchedules);
    mqttClient.subscribe(statusTopic);
  } else {
    Serial.print("Failed to connect to MQTT broker, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 10 seconds");
  }
}

void setup_wifi() {
  WiFi.persistent(false);
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    yield();
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  button.setDebounceTime(100);
  analogWriteFreq(50000);  // Set PWM frequency to 50kHz
  setup_wifi();
  scheduler.begin();
}

void loop() {
  button.loop();
  mqttClient.loop();
  scheduler.loop();

  // low = open, high = closed
  int switchState = !button.getState();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.begin(ssid, password);
  }
  tryConnectMQTT();
  if (state == START) {
    digitalWrite(MOTOR_PIN, HIGH);
    state = UNLOCKING;
  } else if (state == UNLOCKING) {
    if (switchState == LOW) {
      state = ROTATING;
    }
  } else if (state == ROTATING) {
    if (switchState == HIGH) {
      auto currentPortion = scheduler.getCurrentFeedingTime().portion;
      if (rotateCount < currentPortion) {
        rotateCount++;
        state = UNLOCKING;
      } else {
        state = LOCK;
      }
    }
  } else if (state == LOCK) {
    digitalWrite(MOTOR_PIN, LOW);
    state = IDLE;
  } else if (state == IDLE) {
    if (scheduler.isFeedingTime()) {
      Serial.println("Feeding time!");
      auto currentPortion = scheduler.getCurrentFeedingTime().portion;
      Serial.print("Portion: ");
      Serial.println(currentPortion);
      if (currentPortion > 0) {
        startFeeding(false);
      }
    }
    if (Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      Serial.println(command);
      if (command.length() > 0 && command[0] == 'u') {
        startFeeding(false);
      } else if (command.length() > 0 && command[0] == 'f') {
        startFeeding(true);
      }
    }
  }
}
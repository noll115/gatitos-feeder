import mqtt, { MqttClient } from "mqtt";
import { loadLogs } from "./logsHandler";

let mqttClient: MqttClient | null = null;

const mqttURL = process.env.NEXT_PUBLIC_MQTT_HOST;

export const getMqttServerClient = (): MqttClient => {
  if (!mqttURL) {
    throw new Error("MQTT URL is not defined");
  }
  if (!mqttClient) {
    loadLogs();
    mqttClient = mqtt.connect(mqttURL);

    mqttClient.on("connect", () => {
      console.log("Server connected to MQTT broker");
    });

    mqttClient.on("error", (err) => {
      console.error("MQTT connection error:", err);
    });
    mqttClient.subscribe("devices/+/logs");
    mqttClient.on("message", (topic, message) => {
      console.log(`Received message on topic ${topic}: ${message.toString()}`);
    });
  }

  return mqttClient;
};

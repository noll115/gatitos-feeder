import mqtt, { MqttClient } from "mqtt";
import { addLog, loadLogs, LogBody } from "./logsHandler";

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
    mqttClient.on("message", async (topic, message) => {
      const data: LogBody = JSON.parse(message.toString());
      await addLog(data.id, data.message);
    });
  }

  return mqttClient;
};

"use client";
import mqtt from "mqtt";

export const MQTT_STATUS = [
  "CONNECTING",
  "CONNECTED",
  "DISCONNECTED",
  "ERROR",
  "OFFLINE",
  "RECONNECTING",
] as const;

export type MqttStatusType = (typeof MQTT_STATUS)[number];

const mqttURL = process.env.NEXT_PUBLIC_MQTT_HOST;

type MqttParams = {
  setMqttStatus: (status: MqttStatusType) => void;
  setMqttError: (error: Error | mqtt.ErrorWithReasonCode) => void;
};

function createMqttClient({ setMqttStatus, setMqttError }: MqttParams) {
  if (!mqttURL) {
    throw new Error("MQTT URL is not defined");
  }
  const client = mqtt
    .connect(mqttURL, {
      protocol: "wss",
      reconnectPeriod: 5000,
      queueQoSZero: true,
      resubscribe: true,
      clean: true,
      keepalive: 30,
      forceNativeWebSocket: true,
    })
    .on("connect", () => {
      setMqttStatus("CONNECTED");
    })
    .on("error", (error) => {
      setMqttStatus("ERROR");
      console.error(error);
      setMqttError(error);
    })
    .on("disconnect", () => {
      setMqttStatus("DISCONNECTED");
    })
    .on("offline", () => {
      setMqttStatus("OFFLINE");
    })
    .on("reconnect", () => {
      setMqttStatus("RECONNECTING");
    })
    .on("close", () => {
      setMqttStatus("DISCONNECTED");
    });

  return client;
}

export { createMqttClient };

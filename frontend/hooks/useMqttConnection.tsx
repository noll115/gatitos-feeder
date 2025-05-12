"use client";
import { useState, useEffect } from "react";
import { createMqttClient } from "../utils/mqttService";
import mqtt, { MqttClient } from "mqtt";

export type MqttStates =
  | {
      mqttClient: null;
      mqttStatus: "CONNECTING";
      mqttError: null;
    }
  | {
      mqttClient: MqttClient;
      mqttStatus: "CONNECTED" | "DISCONNECTED" | "RECONNECTING" | "OFFLINE";
      mqttError: null;
    }
  | {
      mqttClient: MqttClient;
      mqttStatus: "ERROR";
      mqttError: Error | mqtt.ErrorWithReasonCode;
    };

type MqttStatuses = MqttStates["mqttStatus"];

const isClientState = (
  mqttStatus: MqttStatuses
): mqttStatus is Exclude<MqttStatuses, "CONNECTING" | "ERROR"> => {
  return ["CONNECTED", "DISCONNECTED", "RECONNECTING", "OFFLINE"].includes(
    mqttStatus
  );
};

function useMqttConnection(): MqttStates {
  const [mqttStatus, setMqttStatus] = useState<MqttStatuses>("CONNECTING");

  const [mqttError, setMqttError] = useState<Error | mqtt.ErrorWithReasonCode>(
    {} as Error | mqtt.ErrorWithReasonCode
  );

  const [mqttClient, setMqttClient] = useState<MqttClient | null>(null);

  useEffect(() => {
    const client = createMqttClient({
      setMqttStatus,
      setMqttError,
    });

    setMqttClient(client);

    return () => {
      client.end();
    };
  }, []);

  if (mqttStatus === "ERROR" && mqttClient) {
    return {
      mqttClient,
      mqttStatus,
      mqttError: mqttError,
    };
  }

  if (isClientState(mqttStatus) && mqttClient) {
    return {
      mqttClient,
      mqttStatus: mqttStatus,
      mqttError: null,
    };
  }

  return {
    mqttClient: null,
    mqttStatus: "CONNECTING",
    mqttError: null,
  };
}

export default useMqttConnection;

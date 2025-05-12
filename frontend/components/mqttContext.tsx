//create context for mqtt
"use client";
import { createContext, useContext } from "react";
import useMqttConnection from "@/hooks/useMqttConnection";

const MqttContext = createContext<ReturnType<typeof useMqttConnection>>({
  mqttClient: null,
  mqttStatus: "CONNECTING",
  mqttError: null,
});

export const MqttProvider = ({ children }: { children: React.ReactNode }) => {
  const mqttConnection = useMqttConnection();
  return (
    <MqttContext.Provider value={mqttConnection}>
      {children}
    </MqttContext.Provider>
  );
};

export const useMqttContext = () => {
  const context = useContext(MqttContext);
  if (!context) {
    throw new Error("useMqttContext must be used within a MqttProvider");
  }
  return context;
};

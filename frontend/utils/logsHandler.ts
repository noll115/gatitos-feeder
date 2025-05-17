import fs from "fs/promises";
import path from "path";

export interface LogBody {
  id: string;
  message: string;
}

type Log = {
  time: number;
  message: string;
};

export interface Logs {
  [key: string]: Log[];
}

const logLength = Number(process.env.LOG_LENGTH) ?? 10;
const logsPath = path.join(process.cwd(), "logs", "logs.json");

let logs: Logs = {};

export async function loadLogs() {
  try {
    const data = await fs.readFile(logsPath, "utf-8");
    logs = JSON.parse(data);
  } catch (error: unknown) {
    console.error("Error loading logs:", error);
    await fs.writeFile(logsPath, JSON.stringify({}), "utf-8");
    logs = {};
  }
}

export const addLog = async (id: string, message: string) => {
  if (!logs[id]) {
    logs[id] = [];
  }
  logs[id].unshift({ time: Date.now(), message });
  if (logs[id].length > logLength) {
    logs[id].pop();
  }
  try {
    await fs.writeFile(logsPath, JSON.stringify(logs));
  } catch (error) {
    console.error("Error writing logs:", error);
  }
};

export const getLogs = (id: string) => {
  if (!logs[id]) {
    return [];
  }
  return logs[id];
};

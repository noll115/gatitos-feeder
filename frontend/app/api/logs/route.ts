import { NextRequest, NextResponse } from "next/server";
import fs from "fs/promises";

const corsHeaders = {
  "Access-Control-Allow-Origin": "*", // Allow all origins (replace '*' with specific origin for better security)
  "Access-Control-Allow-Methods": "POST, GET, OPTIONS", // Allowed HTTP methods
  "Access-Control-Allow-Headers": "Content-Type", // Allowed headers
};

interface LogBody {
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

let logs: Logs = {};

const logFilePath = "./logs.json";

async function loadLogs() {
  try {
    const data = await fs.readFile(logFilePath, "utf-8");
    logs = JSON.parse(data);
  } catch (error) {
    console.error("Error loading logs:", error);
  }
}

await loadLogs();

export async function OPTIONS() {
  return new Response(null, {
    status: 204,
    headers: corsHeaders,
  });
}

export async function POST(request: NextRequest) {
  const data: LogBody = await request.json();
  const { id, message } = data;
  console.log("Received log:", data);
  if (!id || !message) {
    return new Response("Missing required fields", {
      status: 400,
      headers: corsHeaders,
    });
  }
  if (!logs[id]) {
    logs[id] = [];
  }
  logs[id].unshift({ time: Date.now(), message });
  if (logs[id].length > logLength) {
    logs[id].pop();
  }
  try {
    await fs.writeFile(logFilePath, JSON.stringify(logs));
  } catch (error) {
    console.error("Error writing logs:", error);
  }
  return new Response(null, {
    status: 200,
    headers: corsHeaders,
  });
}

export async function GET(request: NextRequest) {
  const { searchParams } = new URL(request.url);
  const id = searchParams.get("id");
  if (!id) {
    return new Response("Missing id", {
      status: 400,
    });
  }
  if (!logs[id]) {
    return NextResponse.json([]);
  }
  return NextResponse.json(logs[id]);
}

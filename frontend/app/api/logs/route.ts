import { NextRequest, NextResponse } from "next/server";
import fs from "fs/promises";
import path from "path";
import { getMqttServerClient } from "@/utils/mqttServerService";

const corsHeaders = {
  "Access-Control-Allow-Origin": "*", // Allow all origins (replace '*' with specific origin for better security)
  "Access-Control-Allow-Methods": "POST, GET, OPTIONS", // Allowed HTTP methods
  "Access-Control-Allow-Headers": "Content-Type", // Allowed headers
};

type Log = {
  time: number;
  message: string;
};

export interface Logs {
  [key: string]: Log[];
}

const logsPath = path.join(process.cwd(), "logs", "logs.json");

async function loadLogs() {
  try {
    const data = await fs.readFile(logsPath, "utf-8");
    return JSON.parse(data);
  } catch (error: unknown) {
    console.error("Error loading logs:", error);
    await fs.writeFile(logsPath, JSON.stringify({}), "utf-8");
    return {};
  }
}

export async function OPTIONS() {
  return new Response(null, {
    status: 204,
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
  const logs = await loadLogs();

  if (!logs[id]) {
    return NextResponse.json([]);
  }
  return NextResponse.json(logs[id]);
}

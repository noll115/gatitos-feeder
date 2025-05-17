"use client";
import { useEffect, useMemo, useState } from "react";
import { useMqttContext } from "./mqttContext";
import { ArrowClockwise, PawPrint } from "@phosphor-icons/react";
import { motion } from "framer-motion";
import { ScheduleDisplayInputs } from "./scheduleDisplayInputs";
import Image from "next/image";
import Link from "next/link";
import type { Logs } from "@/utils/logsHandler";

interface FeederProps {
  id: string;
  ref?: React.Ref<HTMLDivElement>;
}

export type EspFeedingSchedule = {
  hour: number;
  minute: number;
  portion: number;
}[];

// extends EspFeedingSchedule and adds id field
export type FeedingSchedule = {
  id: string;
  hour: number;
  minute: number;
  portion: number;
}[];

const getTopicString = (id: string, type: string) => `devices/${id}/${type}`;

const defaultSchedule: (id: string) => FeedingSchedule[number] = (
  id: string
) => ({
  id,
  hour: 0,
  minute: 0,
  portion: 0,
});

const fetchTimeout = 5000;

export const CatFeederController = motion.create(({ id, ref }: FeederProps) => {
  const { mqttClient, mqttError, mqttStatus } = useMqttContext();
  const [feedingSchedule, setFeedingSchedule] = useState<FeedingSchedule>([
    defaultSchedule("0"),
    defaultSchedule("1"),
    defaultSchedule("2"),
  ]);
  const [loadingSchedule, setLoadingSchedule] = useState(true);
  const [timedOut, setTimedOut] = useState(false);
  const topicsListening = useMemo(
    () => ({
      feedingSchedule: getTopicString(id, "feedingSchedule"),
      stateChange: getTopicString(id, "stateChange"),
      ipAddress: getTopicString(id, "ipAddress"),
    }),
    [id]
  );
  const [feederState, setFeederState] = useState<string>("IDLE");
  const [ipAddress, setIpAddress] = useState<string>("");

  const fetchFeedingSchedule = () => {
    if (mqttStatus !== "CONNECTED") return;
    setLoadingSchedule(true);
    mqttClient.publish(getTopicString(id, "getFeedingSchedule"), "");
    setTimeout(() => {
      setLoadingSchedule((prev) => {
        if (prev) {
          setTimedOut(true);
        }
        return false;
      });
    }, fetchTimeout);
  };

  useEffect(() => {
    if (mqttStatus !== "CONNECTED") return;
    mqttClient.on("message", (topic, message) => {
      console.log("Message received:", topic, message.toString());
      if (topic === topicsListening.feedingSchedule) {
        const espSchedule = JSON.parse(
          message.toString()
        ) as EspFeedingSchedule;

        setLoadingSchedule(false);
        setTimedOut(false);
        const localSchedule = espSchedule.map((s, index) => {
          const localDate = new Date();
          localDate.setUTCHours(s.hour);
          localDate.setUTCMinutes(s.minute);
          return {
            id: index.toString() + s.hour + s.minute,
            hour: localDate.getHours(),
            minute: localDate.getMinutes(),
            portion: s.portion,
          };
        });
        console.log("Feeding schedule:", localSchedule);

        setFeedingSchedule(localSchedule);
      }
      if (topic === topicsListening.stateChange) {
        console.log("State change:", message.toString());
        setFeederState(message.toString());
      }
      if (topic === topicsListening.ipAddress) {
        console.log("IP Address:", message.toString());
        setIpAddress(message.toString());
      }
    });
    mqttClient.subscribe([
      topicsListening.feedingSchedule,
      topicsListening.stateChange,
      topicsListening.ipAddress,
    ]);
    fetchFeedingSchedule();
    return () => {
      mqttClient.unsubscribe([
        topicsListening.feedingSchedule,
        topicsListening.stateChange,
        topicsListening.ipAddress,
      ]);
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [mqttClient, mqttStatus, id, topicsListening]);

  const submitSchedule = (e: React.FormEvent) => {
    e.preventDefault();
    if (mqttStatus !== "CONNECTED") return;

    const sentSchedule = feedingSchedule.map((schedule) => {
      const utcDate = new Date();
      utcDate.setHours(schedule.hour);
      utcDate.setMinutes(schedule.minute);

      return {
        hour: utcDate.getUTCHours(),
        minute: utcDate.getUTCMinutes(),
        portion: schedule.portion,
      };
    });
    mqttClient.publish(
      getTopicString(id, "setFeedingSchedule"),
      JSON.stringify(sentSchedule)
    );
  };

  const onScheduleTimeChange = (newSchedule: FeedingSchedule) => {
    // we want to order newSchedule by hour and minute earlies to latest
    const updatedSchedule = [...newSchedule];
    updatedSchedule.sort((a, b) => {
      if (a.hour === b.hour) {
        return a.minute - b.minute;
      }
      return a.hour - b.hour;
    });
    setFeedingSchedule(updatedSchedule);
  };

  const feedCats = () => {
    if (mqttStatus !== "CONNECTED") return;
    mqttClient.publish(getTopicString(id, "command"), "feed");
  };

  if (timedOut) {
    return (
      <div className="rounded-xl bg-base-300 p-4 shadow-md w-full max-w-xl ">
        <span className="flex items-center justify-between gap-2 ">
          <div className="flex items-center gap-2">
            <PawPrint className="size-8" />
            <span className="flex items-end gap-2">
              <h2 className="capitalize text-xl font-semibold leading-none">
                {id}
              </h2>
              <span className="text-sm font-light text-base-content/30 leading-none">
                No response from feeder
              </span>
            </span>
          </div>
          <RefreshButton
            fetchFeedingSchedule={fetchFeedingSchedule}
            loadingSchedule={loadingSchedule}
          />
        </span>

        <div className="flex flex-col items-center justify-center">
          <Image
            src="/kitty.png"
            alt="cat"
            width={256}
            height={256}
          />
        </div>
      </div>
    );
  }

  return (
    <div
      ref={ref}
      className="rounded-xl bg-base-300 p-4 shadow-md w-full max-w-xl"
    >
      <span className="flex items-center justify-between gap-2 ">
        <div className="flex items-center gap-2">
          <PawPrint className="size-8" />
          <span className="flex items-end gap-2">
            <h2 className="capitalize text-xl font-semibold leading-none">
              {id}
            </h2>
            {
              <span className="text-sm font-light text-base-content/30 leading-none">
                {loadingSchedule ? "loading" : feederState}
              </span>
            }
          </span>
        </div>
        <div className="flex gap-2">
          {ipAddress.length > 0 && (
            <Link
              className="btn btn-link"
              href={`http://${ipAddress}/update`}
            >
              Update
            </Link>
          )}
          <RefreshButton
            fetchFeedingSchedule={fetchFeedingSchedule}
            loadingSchedule={loadingSchedule}
          />
        </div>
      </span>
      <form onSubmit={submitSchedule}>
        <ScheduleDisplayInputs
          feedingSchedule={feedingSchedule}
          onScheduleTimeChange={onScheduleTimeChange}
          loading={loadingSchedule}
        />
        <div className="flex justify-between">
          <button
            type="submit"
            disabled={loadingSchedule}
            className="btn btn-primary"
          >
            Submit new schedule
          </button>
          <button
            type="button"
            disabled={loadingSchedule || feederState !== "IDLE"}
            onClick={feedCats}
            className="btn btn-secondary btn-outline"
          >
            Feed Once
          </button>
        </div>
      </form>
      {mqttStatus === "ERROR" && (
        <div className="">
          <p>Error: {JSON.stringify(mqttError)}</p>
        </div>
      )}
      <LogsDisplay id={id} />
    </div>
  );
});

interface RefreshButtonProps {
  fetchFeedingSchedule: () => void;
  loadingSchedule: boolean;
}

const RefreshButton = ({
  fetchFeedingSchedule,
  loadingSchedule,
}: RefreshButtonProps) => {
  return (
    <button
      onClick={fetchFeedingSchedule}
      className="btn group btn-ghost btn-square size-auto p-1 "
      disabled={loadingSchedule}
    >
      <ArrowClockwise
        className={
          "text-secondary group-disabled:opacity-35" +
          (loadingSchedule ? " animate-spin" : "")
        }
      />
      Refresh
    </button>
  );
};

const LogsDisplay = ({ id }: { id: string }) => {
  const [logs, setLogs] = useState<Logs[string]>([]);

  const fetchLogs = async () => {
    const response = await fetch(`/api/logs?id=${id}`);
    if (!response.ok) {
      console.error("Error fetching logs:", response.statusText);
      return;
    }
    const data = (await response.json()) as Logs[string];
    setLogs(data ?? []);
  };

  const handleCheckboxChange = async (
    e: React.ChangeEvent<HTMLInputElement>
  ) => {
    if (e.target.checked) {
      await fetchLogs();
    } else {
      setLogs([]);
    }
  };

  return (
    <div className="collapse collapse-arrow">
      <input
        type="checkbox"
        onChange={handleCheckboxChange}
      />
      <div className="collapse-title font-light">Logs</div>
      <div className="collapse-content text-sm ">
        {logs.length > 0 ? (
          <ul className="flex flex-col gap-2 max-h-36 overflow-y-auto">
            {logs.map((log) => (
              <li
                key={log.time}
                className="flex items-center gap-1"
              >
                <span className="text-xs text-base-content/30">
                  {new Date(log.time).toLocaleString("en-US", {
                    hour: "numeric",
                    minute: "numeric",
                    second: "numeric",
                  })}
                </span>
                <span className="text-sm">{log.message}</span>
              </li>
            ))}
          </ul>
        ) : (
          <div className="flex flex-col gap-2">
            <span className="text-sm text-base-content/30">
              No logs available
            </span>
          </div>
        )}
      </div>
    </div>
  );
};

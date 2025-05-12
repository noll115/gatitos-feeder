import { AnimatePresence } from "motion/react";
import * as motion from "motion/react-client";
import type { FeedingSchedule } from "./catFeederController";
import { useEffect, useRef, useState } from "react";

interface DisplayScheduleProps {
  feedingSchedule: FeedingSchedule;
  onScheduleTimeChange: (schedule: FeedingSchedule) => void;
  loading: boolean;
}

export const ScheduleDisplayInputs = ({
  feedingSchedule,
  onScheduleTimeChange,
  loading,
}: DisplayScheduleProps) => {
  const [focusedInput, setFocusedInput] = useState<string | null>(null);

  if (loading) {
    return (
      <div className="flex flex-col gap-4 p-2 pb-16 pt-4">
        {feedingSchedule.map((schedule, index) => {
          return (
            <div key={schedule.id}>
              <h3 className="text-lg mb-1">Feeding Schedule {index + 1}</h3>
              <div
                key={index + "loading"}
                className="skeleton w-full h-10 rounded"
              ></div>
            </div>
          );
        })}
      </div>
    );
  }

  return (
    <div className="flex flex-col gap-4 p-2 pb-16 pt-4">
      <AnimatePresence mode="sync">
        {feedingSchedule.map((schedule, index) => {
          return (
            <motion.div
              key={schedule.id}
              layout
            >
              <h3 className="text-lg mb-1">Feeding Schedule {index + 1}</h3>
              <TimeScheduleInput
                key={index}
                currentSchedule={schedule}
                focused={focusedInput === schedule.id}
                onFocus={() => setFocusedInput(schedule.id)}
                onBlur={() => setFocusedInput(null)}
                setSchedule={(newSchedule) => {
                  const updatedSchedule = feedingSchedule.map((s, i) => {
                    if (i === index) {
                      return newSchedule;
                    }
                    return s;
                  });
                  onScheduleTimeChange(updatedSchedule);
                }}
              />
            </motion.div>
          );
        })}
      </AnimatePresence>
    </div>
  );
};

interface TimeScheduleInputProps {
  currentSchedule: FeedingSchedule[number];
  setSchedule: (schedule: FeedingSchedule[number]) => void;
  onFocus: () => void;
  onBlur: () => void;
  focused?: boolean;
}

const TimeScheduleInput = ({
  currentSchedule,
  setSchedule,
  onFocus,
  onBlur,
  focused,
}: TimeScheduleInputProps) => {
  const hrStr = currentSchedule.hour.toString().padStart(2, "0");
  const minStr = currentSchedule.minute.toString().padStart(2, "0");
  const ref = useRef<HTMLInputElement>(null);

  useEffect(() => {
    if (focused) {
      ref.current?.focus();
    }
  }, [focused]);

  return (
    <div className="flex justify-between px-2">
      <input
        type="time"
        ref={ref}
        className="input input-bordered w-full max-w-xs"
        value={`${hrStr}:${minStr}`}
        onFocus={onFocus}
        onBlur={onBlur}
        onChange={(e) => {
          const [hour, minute] = e.target.value.split(":");
          setSchedule({
            ...currentSchedule,
            hour: Number(hour),
            minute: Number(minute),
          });
        }}
      />
      <label className="input w-1/4 max-w-xs">
        <span className="label">Portion</span>
        <input
          type="number"
          className="[appearance:textfield] [&::-webkit-outer-spin-button]:appearance-none [&::-webkit-inner-spin-button]:appearance-none"
          value={currentSchedule.portion}
          min={0}
          max={10}
          inputMode="numeric"
          onFocus={(e) => e.target.select()}
          onChange={(e) => {
            setSchedule({
              ...currentSchedule,
              portion: Number(e.target.value),
            });
          }}
        />
      </label>
    </div>
  );
};

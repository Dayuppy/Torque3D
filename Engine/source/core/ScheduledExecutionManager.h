#ifndef SCHEDULED_EXECUTION_MANAGER_H
#define SCHEDULED_EXECUTION_MANAGER_H

#include "console/simBase.h"      // for SimObject
#include "console/consoleTypes.h" // for ConsoleValueArray, StringTableEntry
#include "core/util/tVector.h"

/// Central manager for all Console-scheduled task calls.
class ScheduledExecutionManager
{
   ScheduledExecutionManager();
   ~ScheduledExecutionManager();

public:
   struct Task {
      U32            id;
      SimObject* target;
      String         method;
      Vector<String> args;
      U32            executeTime;     // next fire timestamp
      U32            intervalMs;      // 0 = one-shot, >0 = repeating
      S32            remainingRepeats;// -1 = infinite, >=0 = count
      bool           paused = false;
   };

   /// Get the singleton.
   static ScheduledExecutionManager& instance();

   /// Schedule a one-shot call after delayMs.
   S32  schedule(U32 delayMs, SimObject* target, const char* method,
      U32 argc, const char** argv);

   /// Schedule a repeating call: first after delayMs, then every intervalMs.
   /// Pass maxRepeats = -1 for infinite.
   S32  scheduleRepeating(U32 delayMs, SimObject* target, const char* method,
      U32 intervalMs, S32 maxRepeats,
      U32 argc, const char** argv);

   /// Cancel exactly one task by ID.
   bool cancelTask(S32 taskId);

   /// Cancel all tasks for the given object.
   void cancelAll(SimObject* target);

   /// Pause or resume a single task.
   bool pauseTask(S32 taskId);
   bool resumeTask(S32 taskId);

   /// List all pending tasks.
   void listTasks();

   /// Called once per engine tick to fire off all due tasks.
   void processTasks();

   /// Turn on/off debug logging.
   void setDebug(bool on);

   /// Turn on/off internal profiling.
   void setProfiling(bool on);

   /// Print accumulated profiling stats.
   void dumpProfilingStats();

   /// Coalesce same-method calls within this many ms.
   void  setCoalesceWindow(U32 ms);

   /// Throttle per-target calls per tick.
   void  setMaxTasksPerTarget(U32 maxCalls);

   /// Batch execution across targets by method.
   void  setBatchByMethod(bool on);

private:

   Vector<Task>              mTasks;
   U32                       mNextTaskId = 1;
   bool                      mDebug = false;
   bool                      mProfiling = false;
   U32                       mCoalesceWindowMs = 0;
   U32                       mMaxTasksPerTarget = UINT_MAX;
   bool                      mBatchByMethod = false;

   // Profiling accumulators:
   U64                       mTotalScheduleTime = 0;
   U64                       mTotalDispatchTime = 0;
   U32                       mScheduleCount = 0;
   U32                       mDispatchCount = 0;
};

#endif // SCHEDULED_EXECUTION_MANAGER_H

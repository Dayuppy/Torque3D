#include "ScheduledExecutionManager.h"

#include <algorithm>
#include <map>
#include <unordered_map>

#include "console/console.h"        // Con::printf, Con::executef
#include "console/consoleTypes.h"   // U32Vector, ConsoleValueArray
#include "console/simBase.h"        // Sim::getCurrentTime
#include "console/engineAPI.h"      // DefineEngineFunction

//------------------------------------------------------------------------------
// Singleton
//------------------------------------------------------------------------------
ScheduledExecutionManager& ScheduledExecutionManager::instance()
{
   static ScheduledExecutionManager mgr;
   return mgr;
}

ScheduledExecutionManager::ScheduledExecutionManager()
   : mNextTaskId(1),
   mDebug(false),
   mProfiling(false),
   mCoalesceWindowMs(0),
   mMaxTasksPerTarget(UINT_MAX),
   mBatchByMethod(false),
   mTotalScheduleTime(0),
   mTotalDispatchTime(0),
   mScheduleCount(0),
   mDispatchCount(0)
{
}

ScheduledExecutionManager::~ScheduledExecutionManager() = default;

//------------------------------------------------------------------------------
// schedule (one-shot)
//------------------------------------------------------------------------------
S32 ScheduledExecutionManager::schedule(U32 delayMs,
   SimObject* target, const char* method,
   U32 argc, const char** argv)
{
   if (!target)
   {
      Con::warnf("[SchedMgr] schedule: null target");
      return 0;
   }

   U32 start = Sim::getCurrentTime();

   Task t;
   t.id = mNextTaskId++;
   t.target = target;
   t.method = method;
   t.args.reserve(argc);
   for (U32 i = 0; i < argc; ++i)
      t.args.push_back(argv[i]);
   t.executeTime = start + delayMs;
   t.intervalMs = 0;
   t.remainingRepeats = 0;
   t.paused = false;

   mTasks.push_back(t);

   if (mDebug)
      Con::printf("[SchedMgr] +Task %d -> %s::%s in +%ums",
         t.id, target->getName(), method, delayMs);

   if (mProfiling)
   {
      U32 end = Sim::getCurrentTime();
      mTotalScheduleTime += (end - start);
      ++mScheduleCount;
   }

   return t.id;
}

//------------------------------------------------------------------------------
// scheduleRepeating
//------------------------------------------------------------------------------
S32 ScheduledExecutionManager::scheduleRepeating(U32 delayMs,
   SimObject* target, const char* method,
   U32 intervalMs, S32 maxRepeats,
   U32 argc, const char** argv)
{
   if (!target)
   {
      Con::warnf("[SchedMgr] scheduleRepeating: null target");
      return 0;
   }

   U32 now = Sim::getCurrentTime();

   Task t;
   t.id = mNextTaskId++;
   t.target = target;
   t.method = method;
   t.args.reserve(argc);
   for (U32 i = 0; i < argc; ++i)
      t.args.push_back(argv[i]);
   t.executeTime = now + delayMs;
   t.intervalMs = intervalMs;
   t.remainingRepeats = maxRepeats;
   t.paused = false;

   mTasks.push_back(t);

   if (mDebug)
      Con::printf("[SchedMgr] +RepeatingTask %d -> %s::%s first+%ums every+%ums (%s)",
         t.id, target->getName(), method, delayMs, intervalMs,
         maxRepeats < 0 ? "infinite" : "limited");

   return t.id;
}

//------------------------------------------------------------------------------
// cancel / pause / resume
//------------------------------------------------------------------------------
bool ScheduledExecutionManager::cancelTask(S32 taskId)
{
   for (auto& t : mTasks)
   {
      if (S32(t.id) == taskId)
      {
         if (mDebug)
            Con::printf("[SchedMgr] -Cancelled %d", taskId);
         t.id = 0;
         return true;
      }
   }
   return false;
}

void ScheduledExecutionManager::cancelAll(SimObject* target)
{
   for (auto& t : mTasks)
   {
      if (t.id && t.target == target)
      {
         if (mDebug)
            Con::printf("[SchedMgr] -Cancelled %d on %s",
               t.id, target->getName());
         t.id = 0;
      }
   }
}

bool ScheduledExecutionManager::pauseTask(S32 taskId)
{
   for (auto& t : mTasks)
   {
      if (S32(t.id) == taskId)
      {
         t.paused = true;
         if (mDebug) Con::printf("[SchedMgr] Paused %d", taskId);
         return true;
      }
   }
   return false;
}

bool ScheduledExecutionManager::resumeTask(S32 taskId)
{
   for (auto& t : mTasks)
   {
      if (S32(t.id) == taskId)
      {
         t.paused = false;
         if (mDebug) Con::printf("[SchedMgr] Resumed %d", taskId);
         return true;
      }
   }
   return false;
}

//------------------------------------------------------------------------------
// listTasks
//------------------------------------------------------------------------------
void ScheduledExecutionManager::listTasks()
{
   Con::printf("[SchedMgr] %zu tasks pending:", mTasks.size());
   for (auto& t : mTasks)
   {
      if (!t.id) continue;
      Con::printf("  id=%d on %s::%s @%u%s%s",
         t.id,
         t.target->getName(),
         t.method.c_str(),
         t.executeTime,
         t.intervalMs ? " [repeating]" : "",
         t.paused ? " [paused]" : "");
   }
}

//------------------------------------------------------------------------------
// debug / profiling setters
//------------------------------------------------------------------------------
void ScheduledExecutionManager::setDebug(bool on)
{
   mDebug = on;
   Con::printf("[SchedMgr] Debug %s", on ? "ON" : "OFF");
}

void ScheduledExecutionManager::setProfiling(bool on)
{
   mProfiling = on;
   Con::printf("[SchedMgr] Profiling %s", on ? "ON" : "OFF");
}

void ScheduledExecutionManager::dumpProfilingStats()
{
   Con::printf("[SchedMgr] scheduleCalls=%u totalSchedTime=%llums avg=%llums",
      mScheduleCount,
      mTotalScheduleTime,
      mScheduleCount ? mTotalScheduleTime / mScheduleCount : 0ull);
   Con::printf("[SchedMgr] dispatchCalls=%u totalDispatchTime=%llums avg=%llums",
      mDispatchCount,
      mTotalDispatchTime,
      mDispatchCount ? mTotalDispatchTime / mDispatchCount : 0ull);
}

void ScheduledExecutionManager::setCoalesceWindow(U32 ms)
{
   mCoalesceWindowMs = ms;
   Con::printf("[SchedMgr] CoalesceWindow=%ums", ms);
}

void ScheduledExecutionManager::setMaxTasksPerTarget(U32 n)
{
   mMaxTasksPerTarget = n;
   Con::printf("[SchedMgr] MaxTasksPerTarget=%u", n);
}

void ScheduledExecutionManager::setBatchByMethod(bool on)
{
   mBatchByMethod = on;
   Con::printf("[SchedMgr] BatchByMethod %s", on ? "ON" : "OFF");
}

//------------------------------------------------------------------------------
// processTasks()
//------------------------------------------------------------------------------
void ScheduledExecutionManager::processTasks()
{
   U32 now = Sim::getCurrentTime();

   // 1) gather all due, non‐paused tasks
   Vector<Task> due;
   due.reserve(mTasks.size());
   for (auto& t : mTasks)
      if (t.id && !t.paused && t.executeTime <= now)
         due.push_back(t);

   // 2) remove those due from master list
   for (S32 i = (S32)mTasks.size() - 1; i >= 0; --i)
   {
      Task& t = mTasks[i];
      if (t.id && !t.paused && t.executeTime <= now)
         mTasks.erase(i);
   }

   if (due.empty())
      return;

   // 3) sort by target, method, executeTime
   std::sort(due.begin(), due.end(),
      [&](auto& a, auto& b) {
         if (a.target != b.target) return a.target < b.target;
         if (a.method != b.method) return a.method < b.method;
         return a.executeTime < b.executeTime;
      });

   // 4) coalesce within mCoalesceWindowMs
   struct Bucket { Task t; U32 count; };
   Vector<Bucket> buckets;
   buckets.reserve(due.size());

   for (U32 i = 0; i < due.size(); )
   {
      Task first = due[i];
      U32 windowStart = first.executeTime;
      U32 j = i;
      while (j < due.size()
         && due[j].target == first.target
         && due[j].method == first.method
         && due[j].executeTime <= windowStart + mCoalesceWindowMs)
      {
         ++j;
      }

      Bucket b;
      b.t = due[j - 1];
      b.count = j - i;
      if (mDebug && b.count > 1)
         Con::printf("[SchedMgr] Coalesced %u calls → id %d on %s::%s",
            b.count, b.t.id,
            b.t.target->getName(),
            b.t.method.c_str());
      buckets.push_back(b);
      i = j;
   }

   // 5) throttle per‐target & collect into toExecute
   std::unordered_map<SimObject*, U32> execCount;
   struct ExecItem { SimObject* tgt; String meth; Vector<String> args; };
   Vector<ExecItem> toExecute;
   toExecute.reserve(buckets.size());

   for (auto& b : buckets)
   {
      auto& t = b.t;
      U32& cnt = execCount[t.target];
      if (cnt >= mMaxTasksPerTarget)
      {
         if (mDebug)
            Con::printf("[SchedMgr] Dropping id %d on %s::%s (throttle)",
               t.id, t.target->getName(), t.method.c_str());
         continue;
      }
      ++cnt;
      toExecute.push_back(ExecItem{ t.target, t.method, t.args });
   }

   // 6) dispatch
   U32 dispatchStart = Sim::getCurrentTime();

   if (!mBatchByMethod)
   {
      for (auto& e : toExecute)
      {
         if (mDebug)
            Con::printf("[SchedMgr] Executing %s::%s",
               e.tgt->getName(), e.meth.c_str());

         Vector<const char*> argv;
         argv.reserve(e.args.size());
         for (auto& s : e.args)
            argv.push_back(s.c_str());

         Con::executef(e.tgt,
            e.meth.c_str(),
            S32(argv.size()),
            argv.address());
      }
   }
   else
   {
      // group by method
      std::map<String, Vector<ExecItem>> byMeth;
      for (auto& e : toExecute)
         byMeth[e.meth].push_back(e);

      for (auto& kv : byMeth)
      {
         const String& meth = kv.first;
         auto& items = kv.second;
         if (mDebug)
            Con::printf("[SchedMgr] Bulk invoking %zu calls to %s",
               items.size(), meth.c_str());

         // build flat argv[]
         Vector<const char*> bulk;
         bulk.reserve(2 + items.size() * 3);
         // 1) count
         {
            char buf[16];
            dSprintf(buf, sizeof(buf), "%u", (U32)items.size());
            bulk.push_back(buf);
         }
         // 2) method
         bulk.push_back(meth.c_str());

         // 3) each (id, argCt, args...)
         for (auto& it : items)
         {
            char buf[16];
            dSprintf(buf, sizeof(buf), "%d", it.tgt->getId());
            bulk.push_back(buf);
            dSprintf(buf, sizeof(buf), "%u", (U32)it.args.size());
            bulk.push_back(buf);
            for (auto& s : it.args)
               bulk.push_back(s.c_str());
         }

         Con::executef("_ScheduledExec_bulkInvoke",
            S32(bulk.size()), bulk.address());
      }
   }

   U32 dispatchEnd = Sim::getCurrentTime();
   if (mProfiling)
   {
      mTotalDispatchTime += (dispatchEnd - dispatchStart);
      ++mDispatchCount;
   }

   // 7) reschedule repeats
   for (auto& run : due)
   {
      if (run.intervalMs > 0 && run.remainingRepeats != 0)
      {
         if (run.remainingRepeats > 0)
            --run.remainingRepeats;
         run.executeTime = now + run.intervalMs;
         mTasks.push_back(run);
      }
   }
}

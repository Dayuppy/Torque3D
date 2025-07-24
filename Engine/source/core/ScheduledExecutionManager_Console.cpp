//-----------------------------------------------------------------------------
// ScheduledExecutionManager_Console.cpp
//
// Console bindings for ScheduledExecutionManager
//-----------------------------------------------------------------------------

#include "console/simBase.h"         // Sim::findObject(), Sim::getRootGroup()
#include "console/engineAPI.h"       // DefineEngineStringlyVariadicFunction, DefineEngineFunction
#include "console/console.h"         // Con::printf, Con::executef
#include "core/util/tVector.h"       // Vector<>
#include "ScheduledExecutionManager.h"
#include "console/sim.h"             // for Sim::getCurrentTime()

//-----------------------------------------------------------------------------
// Helper: for any engine‐bound function taking a SimObject*,
//           this will print its ID string (e.g. "42"), or empty string if null.
//-----------------------------------------------------------------------------
const char* castConsoleTypeToString(SimObject* const& obj)
{
   return obj ? obj->getIdString() : "";
}

//-----------------------------------------------------------------------------
// scheduleTask(delayMs, object, method, [arg1...]) -> taskId
//-----------------------------------------------------------------------------
DefineEngineStringlyVariadicFunction(scheduleTask, S32, 3, -1,
   "scheduleTask(delayMs, object, method, [arg1...])\n"
   "@brief Schedule a one‐shot task.\n"
   "@returns A unique task ID, or 0 on error."
)
{
   // parse
   U32 delayMs = (U32)dAtoi(argv[1].getString());
   SimObject* obj = Sim::findObject(argv[2].getString());
   if (!obj) obj = Sim::getRootGroup();
   const char* method = argv[3].getString();

   // collect extra args
   U32 argC = argc > 4 ? argc - 4 : 0;
   Vector<const char*> callArgs;
   callArgs.reserve(argC);
   for (U32 i = 4; i < argc; ++i)
      callArgs.push_back(argv[i].getString());

   // dispatch
   return ScheduledExecutionManager::instance()
      .schedule(delayMs, obj, method, callArgs.size(), callArgs.address());
}

//-----------------------------------------------------------------------------
// scheduleRepeatingTask(delayMs, object, method, intervalMs, maxRepeats, [arg1...]) -> taskId
//-----------------------------------------------------------------------------
DefineEngineStringlyVariadicFunction(scheduleRepeatingTask, S32, 5, -1,
   "scheduleRepeatingTask(delayMs, object, method, intervalMs, maxRepeats, [arg...])\n"
   "@brief Schedule a repeating task.\n"
   "@returns A unique task ID, or 0 on error."
)
{
   // parse
   U32 delayMs = (U32)dAtoi(argv[1].getString());
   SimObject* obj = Sim::findObject(argv[2].getString());
   if (!obj) obj = Sim::getRootGroup();
   const char* method = argv[3].getString();
   U32 intervalMs = (U32)dAtoi(argv[4].getString());
   S32 maxRepeats = dAtoi(argv[5].getString());

   // collect extra args
   U32 argC = argc > 6 ? argc - 6 : 0;
   Vector<const char*> callArgs;
   callArgs.reserve(argC);
   for (U32 i = 6; i < argc; ++i)
      callArgs.push_back(argv[i].getString());

   // dispatch
   return ScheduledExecutionManager::instance()
      .scheduleRepeating(delayMs, obj, method, intervalMs, maxRepeats,
         callArgs.size(), callArgs.address());
}

//-----------------------------------------------------------------------------
// cancelTask(taskId) -> bool
//-----------------------------------------------------------------------------
DefineEngineFunction(cancelTask, bool, (S32 taskId), ,
   "cancelTask(taskId)\n"
   "@brief Cancel a pending task by its ID.\n"
   "@returns True if the task was found and canceled."
)
{
   return ScheduledExecutionManager::instance().cancelTask(taskId);
}

//-----------------------------------------------------------------------------
// cancelAllTasks(objectName)
//-----------------------------------------------------------------------------
DefineEngineFunction(cancelAllTasks, void, (const char* objectName), ,
   "cancelAllTasks(objectName)\n"
   "@brief Cancel all tasks targeting the named object."
)
{
   SimObject* obj = Sim::findObject(objectName);
   if (obj)
      ScheduledExecutionManager::instance().cancelAll(obj);
}

//-----------------------------------------------------------------------------
// pauseTask(taskId) -> bool
//-----------------------------------------------------------------------------
DefineEngineFunction(pauseTask, bool, (S32 taskId), ,
   "pauseTask(taskId)\n"
   "@brief Pause a pending task.\n"
   "@returns True if the task was found and paused."
)
{
   return ScheduledExecutionManager::instance().pauseTask(taskId);
}

//-----------------------------------------------------------------------------
// resumeTask(taskId) -> bool
//-----------------------------------------------------------------------------
DefineEngineFunction(resumeTask, bool, (S32 taskId), ,
   "resumeTask(taskId)\n"
   "@brief Resume a paused task.\n"
   "@returns True if the task was found and resumed."
)
{
   return ScheduledExecutionManager::instance().resumeTask(taskId);
}

//-----------------------------------------------------------------------------
// listTasks()
//-----------------------------------------------------------------------------
DefineEngineFunction(listTasks, void, (), ,
   "listTasks()\n"
   "@brief Dump all pending tasks to the console."
)
{
   ScheduledExecutionManager::instance().listTasks();
}

//-----------------------------------------------------------------------------
// setTaskDebug(on)
//-----------------------------------------------------------------------------
DefineEngineFunction(setTaskDebug, void, (bool on), ,
   "setTaskDebug(on)\n"
   "@brief Enable or disable debug logging in the manager."
)
{
   ScheduledExecutionManager::instance().setDebug(on);
}

//-----------------------------------------------------------------------------
// setTaskProfiling(on)
//-----------------------------------------------------------------------------
DefineEngineFunction(setTaskProfiling, void, (bool on), ,
   "setTaskProfiling(on)\n"
   "@brief Enable or disable internal profiling."
)
{
   ScheduledExecutionManager::instance().setProfiling(on);
}

//-----------------------------------------------------------------------------
// dumpTaskStats()
//-----------------------------------------------------------------------------
DefineEngineFunction(dumpTaskStats, void, (), ,
   "dumpTaskStats()\n"
   "@brief Print scheduling & dispatch profiling statistics."
)
{
   ScheduledExecutionManager::instance().dumpProfilingStats();
}

//-----------------------------------------------------------------------------
// setTaskCoalesceWindow(ms)
//-----------------------------------------------------------------------------
DefineEngineFunction(setTaskCoalesceWindow, void, (S32 ms), ,
   "setTaskCoalesceWindow(ms)\n"
   "@brief Merge calls to the same method/target within this many ms."
)
{
   ScheduledExecutionManager::instance().setCoalesceWindow(ms);
}

//-----------------------------------------------------------------------------
// setMaxTasksPerTarget(n)
//-----------------------------------------------------------------------------
DefineEngineFunction(setMaxTasksPerTarget, void, (S32 n), ,
   "setMaxTasksPerTarget(n)\n"
   "@brief Limit how many calls per target can fire each tick."
)
{
   ScheduledExecutionManager::instance().setMaxTasksPerTarget(n);
}

//-----------------------------------------------------------------------------
// enableTaskBatching(on)
//-----------------------------------------------------------------------------
DefineEngineFunction(enableTaskBatching, void, (bool on), ,
   "enableTaskBatching(on)\n"
   "@brief When enabled, tasks for different targets but the same method\n"
   "       will be grouped into a single bulk console call."
)
{
   ScheduledExecutionManager::instance().setBatchByMethod(on);
}

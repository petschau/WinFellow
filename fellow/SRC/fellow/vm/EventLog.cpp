#include "fellow/vm/EventLog.h"
#include "fellow/debug/log/DebugLogHandler.h"

using namespace std;
using namespace fellow::api::vm;

namespace fellow::vm
{
  vector<EventLogEntry> EventLog::GetEvents()
  {
    vector<EventLogEntry> eventLog;

    for (DebugLogEntry *debugLogEntry : DebugLog.DebugLog)
    {
      eventLog.emplace_back(EventLogEntry(debugLogEntry->FrameNumber, debugLogEntry->Timestamp.Line, debugLogEntry->Timestamp.Pixel, debugLogEntry->GetSource(), debugLogEntry->GetDescription()));
    }

    return eventLog;
  }
}
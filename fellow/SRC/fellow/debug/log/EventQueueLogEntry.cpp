#include <sstream>
#include "fellow/debug/log/EventQueueLogEntry.h"

using namespace std;

const char *EventQueueLogEntry::GetSource() const
{
  return "Event Queue";
}

string EventQueueLogEntry::GetDescription() const
{
  ostringstream oss;

  switch (Reason)
  {
    case EventQueueLogReasons::Pop: oss << "Pop " << EventName; break;
    default: oss << "Unknown enum value for reason"; break;
  }

  return oss.str();
}

EventQueueLogEntry::EventQueueLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, EventQueueLogReasons reason, const char *eventName)
  : DebugLogEntry(source, frameNumber, timestamp), Reason(reason), EventName(eventName)
{
}
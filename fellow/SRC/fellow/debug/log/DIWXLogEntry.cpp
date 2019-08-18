#include <sstream>
#include "fellow/debug/log/DIWXLogEntry.h"

using namespace std;

const char *DIWXLogEntry::GetSource() const
{
  return "DIW X";
}

string DIWXLogEntry::GetDescription() const
{
  ostringstream oss;

  switch (Reason)
  {
    case DIWXLogReasons::DIWX_new_frame: oss << "Initialized for new frame, waiting for start position $" << Start; break;
    case DIWXLogReasons::DIWX_start_found: oss << "Start position $" << Start << " found, now waiting for stop position $" << Stop; break;
    case DIWXLogReasons::DIWX_stop_found: oss << "Stop position $" << Stop << " found, now waiting for start position $" << Start; break;
    case DIWXLogReasons::DIWX_changed: oss << "Start/stop position was changed, start position is $" << Start << ", stop position is $" << Stop; break;
  }

  return oss.str();
}

DIWXLogEntry::DIWXLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DIWXLogReasons reason, uint16_t start, uint16_t stop)
  : DebugLogEntry(source, frameNumber, timestamp), Reason(reason), Start(start), Stop(stop)
{
}

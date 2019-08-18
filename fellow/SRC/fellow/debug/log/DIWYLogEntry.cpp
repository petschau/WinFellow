#include <sstream>
#include "fellow/debug/log/DIWYLogEntry.h"

using namespace std;

const char *DIWYLogEntry::GetSource() const
{
  return "DIW Y";
}

string DIWYLogEntry::GetDescription() const
{
  ostringstream oss;

  switch (Reason)
  {
    case DIWYLogReasons::DIWY_new_frame: oss << "Initialized for new frame, waiting for start line " << Start; break;
    case DIWYLogReasons::DIWY_start_found: oss << "Start line " << Start << " found, now waiting for stop line " << Stop; break;
    case DIWYLogReasons::DIWY_stop_found: oss << "Stop line " << Stop << " found, now waiting for start line " << Start; break;
    case DIWYLogReasons::DIWY_start_changed: oss << "Start line was changed to " << Start << " stop line is " << Stop; break;
    case DIWYLogReasons::DIWY_stop_changed: oss << "Stop line was changed to " << Stop << " start line is " << Start; break;
  }

  return oss.str();
}

DIWYLogEntry::DIWYLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DIWYLogReasons reason, uint16_t start, uint16_t stop)
  : DebugLogEntry(source, frameNumber, timestamp), Reason(reason), Start(start), Stop(stop)
{
}

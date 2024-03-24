#pragma once

#include <string>
#include <vector>

#include "Scheduler/Clocks.h"
#include "Service/IFileops.h"

enum class DebugLogKind
{
  EventHandler,
  CPU
};

struct DebugLogEntry
{
  DebugLogKind Kind;
  uint64_t FrameNumber;
  ChipTimestamp ChipTime;
  std::string Message;
};

class DebugLog
{
private:
  const Clocks &_clocks;
  Service::IFileops &_fileops;
  std::string _filename;
  std::vector<DebugLogEntry> _entries;

  const char *GetKindText(DebugLogKind kind) const;

public:
  bool _enabled = false;

  void Log(DebugLogKind logKind, const char *message);
  void Log(DebugLogKind logKind, const std::string message);
  void Flush();

  DebugLog(const Clocks &clocks, Service::IFileops &fileops);
};

#define DEBUGLOG(kind, message)                                                                                                                                               \
  if (_core.DebugLog->_enabled) _core.DebugLog->Log(kind, message);

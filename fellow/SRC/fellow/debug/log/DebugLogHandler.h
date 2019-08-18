#pragma once

#include "fellow/debug/log/ChipBusLogEntry.h"
#include "fellow/debug/log/DIWXLogEntry.h"
#include "fellow/debug/log/DIWYLogEntry.h"
#include "fellow/debug/log/BitplaneShifterLogEntry.h"
#include "fellow/debug/log/EventQueueLogEntry.h"
#include <vector>

class DebugLogHandler
{
private:
  int _preallocateCount;

  std::vector<ChipBusLogEntry> _chipBusLogEntries;
  std::vector<DIWXLogEntry> _diwxLogEntries;
  std::vector<DIWYLogEntry> _diwyLogEntries;
  std::vector<BitplaneShifterLogEntry> _bitplaneShifterLogEntries;
  std::vector<EventQueueLogEntry> _eventQueueLogEntries;

  void AssertEntryIsLaterThanPrevious(const DebugLogEntry &logEntry);

public:
  bool Enabled;
  std::vector<DebugLogEntry *> DebugLog;

  void AddChipBusLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DebugLogChipBusMode mode, uint32_t address, uint16_t value, uint32_t channel = 0);
  void AddDIWXLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DIWXLogReasons reason, uint16_t start, uint16_t stop);
  void AddDIWYLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DIWYLogReasons reason, uint16_t start, uint16_t stop);
  void AddBitplaneShifterLogEntry(
      DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, BitplaneShifterLogReasons reason, uint32_t outputLine, uint32_t startX, uint32_t pixelCount);
  void AddEventQueueLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, EventQueueLogReasons reason, const char *eventName);

  void PrintLogToFile();
  void Clear();

  DebugLogHandler();
};

extern DebugLogHandler DebugLog;

#include "fellow/api/Services.h"
#include "fellow/debug/log/DebugLogHandler.h"

using namespace std;
using namespace fellow::api;

DebugLogHandler DebugLog;

void DebugLogHandler::Clear()
{
  _chipBusLogEntries.clear();
  _diwxLogEntries.clear();
  _diwyLogEntries.clear();
  _bitplaneShifterLogEntries.clear();
  _eventQueueLogEntries.clear();
  DebugLog.clear();
}

void DebugLogHandler::AssertEntryIsLaterThanPrevious(const DebugLogEntry &logEntry)
{
  if (DebugLog.empty()) return;

  const DebugLogEntry *const previousLogEntry = DebugLog.back();

  F_ASSERT((previousLogEntry->FrameNumber < logEntry.FrameNumber) || ((previousLogEntry->FrameNumber == logEntry.FrameNumber) && (previousLogEntry->Timestamp <= logEntry.Timestamp)));
}

void DebugLogHandler::AddChipBusLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DebugLogChipBusMode mode, uint32_t address, uint16_t value, uint32_t channel)
{
  if (Enabled)
  {
    ChipBusLogEntry &newEntry = _chipBusLogEntries.emplace_back(ChipBusLogEntry(source, frameNumber, timestamp, mode, address, value, channel));

    AssertEntryIsLaterThanPrevious(newEntry);

    DebugLog.push_back(&newEntry);
  }
}

void DebugLogHandler::AddDIWXLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DIWXLogReasons reason, uint16_t start, uint16_t stop)
{
  if (Enabled)
  {
    DIWXLogEntry &newEntry = _diwxLogEntries.emplace_back(DIWXLogEntry(source, frameNumber, timestamp, reason, start, stop));

    AssertEntryIsLaterThanPrevious(newEntry);

    DebugLog.push_back(&newEntry);
  }
}

void DebugLogHandler::AddDIWYLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DIWYLogReasons reason, uint16_t start, uint16_t stop)
{
  if (Enabled)
  {
    DIWYLogEntry &newEntry = _diwyLogEntries.emplace_back(DIWYLogEntry(source, frameNumber, timestamp, reason, start, stop));

    AssertEntryIsLaterThanPrevious(newEntry);

    DebugLog.push_back(&newEntry);
  }
}

void DebugLogHandler::AddBitplaneShifterLogEntry(
    DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, BitplaneShifterLogReasons reason, uint32_t outputLine, uint32_t startX, uint32_t pixelCount)
{
  if (Enabled)
  {
    BitplaneShifterLogEntry &newEntry = _bitplaneShifterLogEntries.emplace_back(BitplaneShifterLogEntry(source, frameNumber, timestamp, reason, outputLine, startX, pixelCount));

    AssertEntryIsLaterThanPrevious(newEntry);

    DebugLog.push_back(&newEntry);
  }
}

void DebugLogHandler::AddEventQueueLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, EventQueueLogReasons reason, const char *eventName)
{
  if (Enabled)
  {
    EventQueueLogEntry &newEntry = _eventQueueLogEntries.emplace_back(EventQueueLogEntry(source, frameNumber, timestamp, reason, eventName));

    AssertEntryIsLaterThanPrevious(newEntry);

    DebugLog.push_back(&newEntry);
  }
}

void DebugLogHandler::PrintLogToFile()
{
  constexpr auto descriptionBufferLength = 2048;
  char descriptionBuffer[descriptionBufferLength];
  char filename[CFG_FILENAME_LENGTH];
  Service->Fileops.GetGenericFileName(filename, "WinFellow", "event.log");
  FILE *F = fopen(filename, "w");

  for (DebugLogEntry *logEntry : DebugLog)
  {
    string entryDescription = logEntry->GetDescription();
    fprintf(F, "%llu\t%u\t%u\t%s\t%.*s\n", logEntry->FrameNumber, logEntry->Timestamp.Line, logEntry->Timestamp.Pixel, logEntry->GetSource(), descriptionBufferLength, entryDescription.c_str());
  }
  fclose(F);
}

DebugLogHandler::DebugLogHandler() : Enabled(false), _preallocateCount(100000)
{
  DebugLog.reserve(_preallocateCount);
  _chipBusLogEntries.reserve(_preallocateCount);
  _diwxLogEntries.reserve(_preallocateCount);
  _diwyLogEntries.reserve(_preallocateCount);
  _bitplaneShifterLogEntries.reserve(_preallocateCount);
  _eventQueueLogEntries.reserve(_preallocateCount);
}

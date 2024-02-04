#include "DebugLog.h"

using namespace Service;
using namespace std;

void DebugLog::Log(DebugLogKind kind, const char *message)
{
  if (!_enabled) return;

  _entries.emplace_back(DebugLogEntry{
      .Kind = kind,
      .FrameNumber = _timekeeper.GetFrameNumber(),
      .AgnusLine = _timekeeper.GetAgnusLine(),
      .AgnusLineCycle = _timekeeper.GetAgnusLineCycle(),
      .Message = message});
}

void DebugLog::Log(DebugLogKind kind, const string message)
{
  if (!_enabled) return;

  _entries.emplace_back(DebugLogEntry{
      .Kind = kind,
      .FrameNumber = _timekeeper.GetFrameNumber(),
      .AgnusLine = _timekeeper.GetAgnusLine(),
      .AgnusLineCycle = _timekeeper.GetAgnusLineCycle(),
      .Message = message});
}

const char *DebugLog::GetKindText(DebugLogKind kind) const
{
  switch (kind)
  {
    case DebugLogKind::EventHandler: return "Event";
    case DebugLogKind::CPU: return "CPU";
  }

  return "Unknown DebugLogKind";
}

void DebugLog::Flush()
{
  if (!_enabled) return;

  FILE *F = fopen(_filename.c_str(), "a");
  if (F == nullptr)
  {
    return;
  }

  for (const DebugLogEntry &entry : _entries)
  {
    fprintf(F, "%s: %s (%I64d, %d, %d)\n", GetKindText(entry.Kind), entry.Message.c_str(), entry.FrameNumber, entry.AgnusLine, entry.AgnusLineCycle);
  }

  fclose(F);

  _entries.clear();
}

DebugLog::DebugLog(const Timekeeper &timekeeper, IFileops &fileops) : _timekeeper(timekeeper), _fileops(fileops), _filename()
{
  char filename[256];
  _fileops.GetGenericFileName(filename, "WinFellow", "Debug.log");
  _filename = filename;

  FILE *F = fopen(_filename.c_str(), "w");
  if (F == nullptr)
  {
    return;
  }

  fclose(F);
}
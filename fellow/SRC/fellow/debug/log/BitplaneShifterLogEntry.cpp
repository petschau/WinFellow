#include <sstream>
#include "fellow/debug/log/BitplaneShifterLogEntry.h"

using namespace std;

const char *BitplaneShifterLogEntry::GetSource() const
{
  return "Bitplane shifter";
}

string BitplaneShifterLogEntry::GetDescription() const
{
  ostringstream oss;

  switch (Reason)
  {
    case BitplaneShifterLogReasons::BitplaneShifter_output_until:
      oss << "Output pixels for line " << OutputLine << " from " << StartX << " to " << StartX + PixelCount - 1 << " total " << PixelCount;
      break;
    case BitplaneShifterLogReasons::BitplaneShifter_duplicate: oss << "Duplicate flush for line " << OutputLine << " from " << StartX; break;
  }

  return oss.str();
}

BitplaneShifterLogEntry::BitplaneShifterLogEntry(
    DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, BitplaneShifterLogReasons reason, uint32_t outputLine, uint32_t startX, uint32_t pixelCount)
  : DebugLogEntry(source, frameNumber, timestamp), Reason(reason), OutputLine(outputLine), StartX(startX), PixelCount(pixelCount)
{
}
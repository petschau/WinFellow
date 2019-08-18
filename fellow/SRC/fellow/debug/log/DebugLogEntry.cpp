#include "fellow/debug/log/DebugLogEntry.h"

DebugLogEntry::DebugLogEntry(DebugLogSource source, ULL frameNumber, const SHResTimestamp &timestamp) : Source(source), FrameNumber(frameNumber), Timestamp(timestamp)
{
}

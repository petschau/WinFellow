#include <sstream>
#include "fellow/debug/log/ChipBusLogEntry.h"

using namespace std;

const char *ChipBusLogEntry::GetSource() const
{
  switch (Source)
  {
    case DebugLogSource::CHIP_BUS_BITPLANE_DMA: return "Bitplane DMA";
    case DebugLogSource::CHIP_BUS_COPPER_DMA: return "Copper DMA";
    case DebugLogSource::CHIP_BUS_SPRITE_DMA: return "Sprite DMA";
    default: break;
  }

  return "Unknown/Bug";
}

string ChipBusLogEntry::GetDescription() const
{
  ostringstream oss;

  switch (Source)
  {
    case DebugLogSource::CHIP_BUS_SPRITE_DMA: oss << "Channel " << Channel << " read " << Value << " from " << Address; break;
    case DebugLogSource::CHIP_BUS_BITPLANE_DMA: oss << "Channel " << Channel << " read " << Value << " from " << Address; break;
    case DebugLogSource::CHIP_BUS_COPPER_DMA:
      if (Mode != DebugLogChipBusMode::Null)
      {
        oss << GetModeString() << " " << Value << " " << GetFromToString() << " " << Address;
      }
      else
      {
        oss << "Null / Allocated / Reloading";
      }
      break;
    default: oss << "!! Unknown chip bus source !!"; break;
  }

  return oss.str();
}

const char *ChipBusLogEntry::GetFromToString() const
{
  return Mode == DebugLogChipBusMode::Read ? "from" : "to";
}

const char *ChipBusLogEntry::GetModeString() const
{
  switch (Mode)
  {
    case DebugLogChipBusMode::Read: return "Read";
    case DebugLogChipBusMode::Write: return "Write";
    case DebugLogChipBusMode::Null: return "Null";
  }

  return "Bug/Unknown";
}

ChipBusLogEntry::ChipBusLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DebugLogChipBusMode mode, uint32_t address, uint16_t value, uint32_t channel)
  : DebugLogEntry(source, frameNumber, timestamp), Mode(mode), Address(address), Value(value), Channel(channel)
{
}
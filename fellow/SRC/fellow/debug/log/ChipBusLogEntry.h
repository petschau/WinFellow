#pragma once

#include "fellow/debug/log/DebugLogEntry.h"

enum class DebugLogChipBusMode
{
  Read,
  Write,
  Null
};

class ChipBusLogEntry : public DebugLogEntry
{
private:
  const char *GetModeString() const;
  const char *GetFromToString() const;

public:
  DebugLogChipBusMode Mode;
  uint32_t Channel;
  uint32_t Address;
  uint16_t Value;

  const char *GetSource() const override;
  std::string GetDescription() const override;

  ChipBusLogEntry(DebugLogSource source, uint64_t frameNumber, const SHResTimestamp &timestamp, DebugLogChipBusMode mode, uint32_t address, uint16_t value, uint32_t channel);
  virtual ~ChipBusLogEntry() = default;
};

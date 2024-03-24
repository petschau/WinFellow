#pragma once

#include <cstdint>

#include "FrameParameters.h"
#include "ChipTimestamp.h"
#include "MasterTimestamp.h"

class Clocks
{
private:
  FrameParameters _frameParameters;

  uint64_t _frameNumber{};
  MasterTimestamp _masterTime;
  ChipTimestamp _chipTime{};

  uint32_t _deniseLineCycle{};

public:
  uint64_t GetFrameNumber() const;

  MasterTimestamp GetMasterTime() const;
  //uint32_t GetFrame280nsCycle() const;

  ChipTimestamp GetChipTime() const;
  uint32_t GetChipLine() const;
  uint32_t GetChipCycle() const;

  uint32_t GetDeniseLineCycle() const;

//  constexpr static uint32_t GetCycleFrom280ns(uint32_t cycle280ns);
  constexpr static uint32_t ToMasterCycleCount(uint32_t chipCycles);
  constexpr static MasterTimestamp ToMasterTime(ChipTimestamp chipTime);

  //static uint32_t GetMasterCycleTotalFrom280ns(uint32_t cycle280ns);

  void SetMasterTime(MasterTimestamp newMasterTime);

  void Clear(const FrameParameters &frameParameters);
  void NewFrame(const FrameParameters &frameParameters);

  void HardReset(const FrameParameters &frameParameters);
};

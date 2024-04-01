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
  ChipTimestamp GetChipTime() const;

  uint32_t GetDeniseLineCycle() const;

  constexpr static MasterTimeOffset ToMasterTimeOffset(const ChipTimeOffset chipTimeOffset);
  MasterTimestamp ToMasterTime(const ChipTimestamp chipTime) const;

  void SetMasterTime(MasterTimestamp newMasterTime);

  void Clear(const FrameParameters &frameParameters);
  void NewFrame(const FrameParameters &frameParameters);

  void HardReset(const FrameParameters &frameParameters);
};

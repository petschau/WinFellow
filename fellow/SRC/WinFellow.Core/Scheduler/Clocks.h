#pragma once

#include <cstdint>

#include "FrameParameters.h"

class Clocks
{
private:
  FrameParameters _frameParameters;

  uint64_t _frameNumber{};
  uint32_t _frameCycle{};
  uint32_t _agnusLine{};
  uint32_t _agnusLineCycle{};
  uint32_t _deniseLineCycle{};

public:
  uint64_t GetFrameNumber() const;
  uint32_t GetFrameCycle() const;
  uint32_t GetAgnusLine() const;
  uint32_t GetAgnusLineCycle() const;
  uint32_t GetDeniseLineCycle() const;

  static uint32_t GetCycleFrom280ns(uint32_t cycle280ns);

  void SetFrameCycle(uint32_t newCycle);

  void Clear(const FrameParameters &frameParameters);
  void NewFrame(const FrameParameters &frameParameters);

  void HardReset(const FrameParameters &frameParameters);
};

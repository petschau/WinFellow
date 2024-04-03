#include "Clocks.h"
#include "ClockConstants.h"

#include <cassert>

uint64_t Clocks::GetFrameNumber() const
{
  return _frameNumber;
}

MasterTimestamp Clocks::GetMasterTime() const
{
  return _masterTime;
}

ChipTimestamp Clocks::GetChipTime() const
{
  return _chipTime;
}

uint32_t Clocks::GetDeniseLineCycle() const
{
  return _deniseLineCycle;
}

MasterTimestamp Clocks::ToMasterTime(const ChipTimestamp chipTimestamp) const
{
  return MasterTimestamp{.Cycle = chipTimestamp.Line * _frameParameters.LongLineMasterCycles.Offset + chipTimestamp.Cycle * ClockConstants::MasterCyclesInChipCycle};
}

constexpr MasterTimeOffset Clocks::ToMasterTimeOffset(const ChipTimeOffset chipTimeOffset)
{
  return MasterTimeOffset{.Offset = chipTimeOffset.Offset * ClockConstants::MasterCyclesInChipCycle};
}

constexpr MasterTimeOffset Clocks::ToMasterTimeOffset(const CpuTimeOffset cpuTimeOffset)
{
  return MasterTimeOffset{.Offset = cpuTimeOffset.Offset * ClockConstants::MasterCyclesInCpuCycle};
}

void Clocks::SetMasterTime(MasterTimestamp newMasterTime)
{
  MasterTimestamp previousMasterTime = _masterTime;

  assert(previousMasterTime <= newMasterTime);

  _masterTime = newMasterTime;
  _chipTime = ChipTimestamp::FromMasterTime(newMasterTime, _frameParameters);
  _deniseLineCycle = newMasterTime.Cycle % _frameParameters.LongLineMasterCycles.Offset;
}

void Clocks::Clear(const FrameParameters &frameParameters)
{
  _frameParameters = frameParameters;
  _masterTime.Clear();
  _chipTime.Clear();
  _deniseLineCycle = 0;
}

void Clocks::NewFrame(const FrameParameters &frameParameters)
{
  _frameParameters = frameParameters;
  _frameNumber++;
  _masterTime.Clear();
  _chipTime.Clear();
  _deniseLineCycle = 0;
}

void Clocks::HardReset(const FrameParameters &frameParameters)
{
  NewFrame(frameParameters);
}

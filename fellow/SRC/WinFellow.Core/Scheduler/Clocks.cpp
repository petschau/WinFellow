#include "Clocks.h"

#include <cassert>

uint64_t Clocks::GetFrameNumber() const
{
  return _frameNumber;
}

MasterTimestamp Clocks::GetMasterTime() const
{
  return _masterTime;
}

//uint32_t Clocks::GetChipCycleFrin() const
//{
//  return _frameMasterCycle >> MasterCycleFrom280nsShift;
//}

ChipTimestamp Clocks::GetChipTime() const
{
  return _chipTime;
}

uint32_t Clocks::GetChipLine() const
{
  return _chipTime.Line;
}

uint32_t Clocks::GetChipCycle() const
{
  return _chipTime.Cycle;
}

uint32_t Clocks::GetDeniseLineCycle() const
{
  return _deniseLineCycle;
}

//constexpr uint32_t Clocks::GetCycleFrom280ns(uint32_t cycle280ns)
//{
//  return cycle280ns;
//}

constexpr uint32_t Clocks::ToMasterCycleCount(uint32_t chipCycles)
{
  return chipCycles << MasterCycleFromChipCycleShift;
}

//uint32_t Clocks::GetMasterCycleTotalFrom280ns(uint32_t cycle280ns)
//{
//  return (cycle280ns + 1) << MasterCycleFrom280nsShift;
//}

void Clocks::SetMasterTime(MasterTimestamp newMasterTime)
{
  MasterTimestamp previousMasterTime = _masterTime;

  assert(previousMasterTime <= newMasterTime);

  _masterTime = newMasterTime;
  _chipTime = ChipTimestamp::FromMasterTime(newMasterTime, _frameParameters);
  _deniseLineCycle = newMasterTime.Cycle % _frameParameters.LongLineCycles;
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

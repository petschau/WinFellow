#include "Timekeeper.h"

#include <cassert>

uint64_t Timekeeper::GetFrameNumber() const
{
  return _frameNumber;
}

uint32_t Timekeeper::GetFrameCycle() const
{
  return _frameCycle;
}

uint32_t Timekeeper::GetAgnusLine() const
{
  return _agnusLine;
}

uint32_t Timekeeper::GetAgnusLineCycle() const
{
  return _agnusLineCycle;
}

uint32_t Timekeeper::GetDeniseLineCycle() const
{
  return _deniseLineCycle;
}

uint32_t Timekeeper::GetCycleFrom280ns(uint32_t cycle280ns)
{
  return cycle280ns;
}

void Timekeeper::SetFrameCycle(uint32_t newCycle)
{
  uint32_t previousCycle = _frameCycle;

  assert(previousCycle <= newCycle);

  _frameCycle = newCycle;
  _agnusLine = newCycle / _frameParameters.LongLineCycles;
  _agnusLineCycle = newCycle % _frameParameters.LongLineCycles;
  _deniseLineCycle = newCycle % _frameParameters.LongLineCycles;
}

void Timekeeper::Clear(const FrameParameters &frameParameters)
{
  _frameParameters = frameParameters;
  _frameCycle = 0;
  _agnusLine = 0;
  _agnusLineCycle = 0;
  _deniseLineCycle = 0;
}

void Timekeeper::NewFrame(const FrameParameters &frameParameters)
{
  _frameParameters = frameParameters;
  _frameNumber++;
  _frameCycle = 0;
  _agnusLine = 0;
  _agnusLineCycle = 0;
  _deniseLineCycle = 0;
}

void Timekeeper::HardReset(const FrameParameters &frameParameters)
{
  NewFrame(frameParameters);
}

#pragma once

class IAgnus
{
public:
  virtual void SetFrameParameters(bool isLongFrame) = 0;

  virtual void EndOfLine() = 0;
  virtual void EndOfFrame() = 0;

  virtual void InitializeEndOfLineEvent() = 0;
  virtual void InitializeEndOfFrameEvent() = 0;

  virtual void HardReset() = 0;

  virtual ~IAgnus() = default;
};

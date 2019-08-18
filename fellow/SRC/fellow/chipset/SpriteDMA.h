#pragma once

#include "fellow/api/defs.h"
#include "fellow/time/Timestamps.h"

struct SpriteDMAChannel
{
  UWO VStart;
  UWO VStop;
  bool ReadFirstWord;
  bool ReadData;
  bool ReadControl;

  bool WantsData() const;
};

class SpriteDMA
{
private:
  SpriteDMAChannel _channel[8];
  const ULO InitialBaseClockCycleForDMACheck;
  unsigned int _currentChannel;

  void ReadControlWord(unsigned int spriteNumber, const ChipBusTimestamp &currentTime, UWO value);
  void ReadDataWord(unsigned int spriteNumber, const ChipBusTimestamp &currentTime, UWO value);

  bool IsInVerticalBlank(ULO line);
  bool IsFirstLineAfterVerticalBlank(ULO line);
  void IncreasePtr(unsigned int spriteNumber);
  void UpdateSpriteDMAState(unsigned int spriteNumber, ULO line);
  void InitiateDMASequence(ULO line);
  void ScheduleEvent(ULO baseClockTimestamp);

  void Clear();

public:
  static void HandleEvent();
  void Handle();

  static void DMAReadCallback(const ChipBusTimestamp &currentTime, UWO value);
  void ReadCallback(const ChipBusTimestamp &currentTime, UWO value);

  void SetVStartLow(unsigned int spriteNumber, UWO vstartLow);
  void SetVStartHigh(unsigned int spriteNumber, UWO vstartHigh);
  void SetVStop(unsigned int spriteNumber, UWO vstop);

  void EndOfLine();
  void EndOfFrame();

  SpriteDMA();
  ~SpriteDMA() = default;
};

extern SpriteDMA sprite_dma;

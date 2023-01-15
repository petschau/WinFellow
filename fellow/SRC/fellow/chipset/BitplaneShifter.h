#pragma once

#include "fellow/api/defs.h"
#include "fellow/api/service/IPerformanceCounter.h"
#include "fellow/time/Timestamps.h"
#include "fellow/chipset/BitplaneUtility.h"

// Keeps track of the shifters for each bitplane
// Uses a batch system that holds a list of armed bitplane words and the position the words were armed for.
// An entire batch is shifted using the same shift mode, ie changing from lores to hires will flush the current batch and
// start a new one. Other chip events might also flush the batch. Eventually end of line will also flush.
enum BitplaneShiftMode
{
  SHIFTMODE_140NS = 0,
  SHIFTMODE_70NS = 1,
  SHIFTMODE_35NS = 2
};

struct BitplaneArmData
{
  ULO Number;
  UWO Data;
};

struct BitplaneArmEntry
{
  ULO X;
  ULO Count;
  BitplaneArmData Data[6];
};

constexpr int ArmListSize = 128;

struct BitplaneArmList
{
  unsigned int EntryCount;
  unsigned int First;
  BitplaneArmEntry Entries[ArmListSize];

  void Clear();
  BitplaneArmEntry &GetFirst();
  BitplaneArmEntry &AllocateNext();
};

class BitplaneShifter
{
private:
  Scheduler *_scheduler;
  BitplaneRegisters *_bitplaneRegisters;
  BitplaneArmList _armList;

  ByteWordUnion _input[6];

  ULO _lastOutputX;
  bool _activated;

  ULO CalculateArmX(ULO baseX, ULO waitMask, bool isLores);
  void AddArmEntry(unsigned int first, unsigned int stride, ULO armX, const UWO *dat);
  ULO GetNextArmSplitX(ULO batchEndX);
  void ActivateArmedBitplaneData(ULO x);

  void AddOutputUntilLogEntry(ULO outputLine, ULO startX, ULO pixelCount);
  void AddDuplicateLogEntry(ULO outputLine, ULO startX);

  void SetupEvent(ULO line, ULO cycle);
  void ShiftActive(ULO pixelCount);
  void ClearActive();

  void ShiftPixels(ULO pixelCount, bool isVisible);
  void ShiftPixelsDual(ULO pixelCount, bool isVisible);

  void ShiftSubBatch(ULO pixelCount, bool isVisible);
  static void ShiftBackgroundBatch(ULO pixelCount);
  void ShiftBitplaneBatch(ULO pixelCount);

  void InitializeNewLine(const SHResTimestamp &currentSHResPosition);
  void InitializeNewFrame();
  void Clear();
  void Reset();

  void NewChangelistBatch(ULO line, ULO pixel) const;

public:
  fellow::api::IPerformanceCounter *PerformanceCounter;

  void AddData(const UWO *dat);
  void Flush();
  void Flush(const SHResTimestamp &untilPosition);

  static void HandleEvent();
  void Handler();

  void UpdateDrawBatchFunc();

  void EndOfFrame();
  void SoftReset();
  void HardReset();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  BitplaneShifter(Scheduler *scheduler, BitplaneRegisters *bitplaneRegisters);
  ~BitplaneShifter();
};

extern BitplaneShifter bitplane_shifter;

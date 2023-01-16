#pragma once

#include "fellow/api/defs.h"
#include "fellow/chipset/BitplaneDMAFetchPrograms.h"
#include "fellow/time/Timestamps.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/chipset/BitplaneRegisters.h"

class DDFStateMachine;

class BitplaneDMA
{
private:
  Scheduler *_scheduler{};
  BitplaneRegisters *_bitplaneRegisters{};
  DDFStateMachine *_ddfStateMachine{};

  BitplaneFetchPrograms FetchPrograms;
  BitplaneFetchProgramRunner FetchProgramRunner;
  bool IsLastFetch{};

  ULO GetModuloForBitplane(unsigned int bitplaneIndex);

  bool CanFetch();
  bool CalculateIsLastFetch(ULO atCycle);

  void ScheduleFetch(const ChipBusTimestamp &currentTime);
  void ScheduleInhibitedFetch(const ChipBusTimestamp &fetchTime);
  void InhibitedReadCallback(const ChipBusTimestamp &currentTime);

public:
  void HandleEvent();

  void DMAReadCallback(const ChipBusTimestamp &currentTime, UWO value);
  void StartFetchProgram(const ChipBusTimestamp &startTime);
  void EndOfFrame();

  BitplaneDMA(Scheduler *scheduler, BitplaneRegisters *bitplaneRegisters, DDFStateMachine *ddfStateMachine);
};

#pragma once

#include "fellow/api/defs.h"
#include "fellow/chipset/BitplaneDMAFetchPrograms.h"
#include "fellow/time/Timestamps.h"

class BitplaneDMA
{
private:
  static BitplaneFetchPrograms FetchPrograms;
  static BitplaneFetchProgramRunner FetchProgramRunner;
  static bool IsLastFetch;

  static ULO GetModuloForBitplane(unsigned int bitplaneIndex);

  static bool CanFetch();
  static bool CalculateIsLastFetch(ULO atCycle);

  static void ScheduleFetch(const ChipBusTimestamp &currentTime);
  static void ScheduleInhibitedFetch(const ChipBusTimestamp &fetchTime);
  static void InhibitedReadCallback(const ChipBusTimestamp &currentTime);

public:
  static void HandleEvent();

  static void DMAReadCallback(const ChipBusTimestamp &currentTime, UWO value);
  static void StartFetchProgram(const ChipBusTimestamp &startTime);
  static void EndOfFrame();
};

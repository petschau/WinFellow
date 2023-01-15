#include "fellow/chipset/BitplaneDMA.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/BitplaneUtility.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/DMAController.h"
#include "fellow/chipset/DIWYStateMachine.h"
#include "fellow/chipset/DDFStateMachine.h"
#include "fellow/scheduler/Scheduler.h"

BitplaneFetchPrograms BitplaneDMA::FetchPrograms;
BitplaneFetchProgramRunner BitplaneDMA::FetchProgramRunner;
bool BitplaneDMA::IsLastFetch;

ULO BitplaneDMA::GetModuloForBitplane(unsigned int bitplaneIndex)
{
  return (ULO)(LON)(WOR)((bitplaneIndex & 1) ? bitplane_registers.bpl2mod : bitplane_registers.bpl1mod);
}

void BitplaneDMA::DMAReadCallback(const ChipBusTimestamp &currentTime, UWO value)
{
  unsigned int bitplaneIndex = FetchProgramRunner.GetBitplaneIndex();

  bitplane_registers.SetBplDat(bitplaneIndex, value);
  bitplane_registers.AddBplPt(bitplaneIndex, 2);

  if (IsLastFetch && FetchProgramRunner.GetCanAddModulo())
  {
    bitplane_registers.AddBplPt(bitplaneIndex, GetModuloForBitplane(bitplaneIndex));
  }

  FetchProgramRunner.NextStep();

  if (FetchProgramRunner.IsEnded())
  {
    if (!IsLastFetch && ddf_state_machine.IsFetchEnabled())
    {
      ChipBusTimestamp startTime(currentTime);
      startTime.Add(1);
      StartFetchProgram(startTime);
    }
    else
    {
      IsLastFetch = false;
    }
  }
  else
  {
    // Continue program
    ScheduleFetch(currentTime);
  }
}

void BitplaneDMA::InhibitedReadCallback(const ChipBusTimestamp &currentTime)
{
  unsigned int bitplaneIndex = FetchProgramRunner.GetBitplaneIndex();

  if (IsLastFetch && FetchProgramRunner.GetCanAddModulo())
  {
    bitplane_registers.AddBplPt(bitplaneIndex, GetModuloForBitplane(bitplaneIndex));
  }

  FetchProgramRunner.NextStep();

  if (FetchProgramRunner.IsEnded())
  {
    if (!IsLastFetch && ddf_state_machine.IsFetchEnabled())
    {
      ChipBusTimestamp startTime(currentTime);
      startTime.Add(1);
      StartFetchProgram(startTime);
    }
    else
    {
      IsLastFetch = false;
    }
  }
  else
  {
    // Continue program
    ScheduleFetch(currentTime);
  }
}

void BitplaneDMA::ScheduleInhibitedFetch(const ChipBusTimestamp &fetchTime)
{
  bitplaneDMAEvent.cycle = fetchTime.ToBaseClockTimestamp();
  scheduler.InsertEvent(&bitplaneDMAEvent);
}

void BitplaneDMA::ScheduleFetch(const ChipBusTimestamp &currentTime)
{
  ChipBusTimestamp fetchTime(currentTime);
  fetchTime.Add(FetchProgramRunner.GetCycleOffset());

  if (fetchTime.GetCycle() >= 0xe0 || fetchTime.GetCycle() < 0x18)
  {
    ScheduleInhibitedFetch(fetchTime);
  }
  else
  {
    const unsigned int bitplaneIndex = FetchProgramRunner.GetBitplaneIndex();
    dma_controller.ScheduleBitplaneDMA(fetchTime, bitplane_registers.bplpt[bitplaneIndex], bitplaneIndex);
  }
}

bool BitplaneDMA::CanFetch()
{
  return BitplaneUtility::IsBitplaneDMAEnabled() && BitplaneUtility::GetEnabledBitplaneCount() > 0 && diwy_state_machine.IsVisible() &&
         ddf_state_machine.IsFetchEnabled();
}

bool BitplaneDMA::CalculateIsLastFetch(ULO atCycle)
{
  // Either ddf says this is the last one because stop has been seen
  if (ddf_state_machine.IsNextFetchLast(atCycle))
  {
    return true;
  }

  // Or this is OCS and we ran into the hard stop that ends the line
  return (!chipsetGetECS() && (atCycle >= (chipsetGetMaximumDDF() - 4U)));
}

void BitplaneDMA::HandleEvent()
{
  bitplaneDMAEvent.Disable();
  InhibitedReadCallback(scheduler.GetChipBusTimestamp());
}

void BitplaneDMA::StartFetchProgram(const ChipBusTimestamp &startTime)
{
  if (CanFetch())
  {
    IsLastFetch = CalculateIsLastFetch(startTime.GetCycle());
    FetchProgramRunner.StartProgram(FetchPrograms.GetFetchProgram(BitplaneUtility::IsLores(), BitplaneUtility::GetEnabledBitplaneCount()));

    if (!FetchProgramRunner.IsEnded())
    {
      ScheduleFetch(startTime);
    }
  }
}

/* Fellow events */

void BitplaneDMA::EndOfFrame()
{
}

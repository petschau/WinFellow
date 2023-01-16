#include "fellow/chipset/BitplaneDMA.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/BitplaneUtility.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/DMAController.h"
#include "fellow/chipset/DIWYStateMachine.h"
#include "fellow/chipset/DDFStateMachine.h"
#include "fellow/scheduler/Scheduler.h"

ULO BitplaneDMA::GetModuloForBitplane(unsigned int bitplaneIndex)
{
  return (ULO)(LON)(WOR)((bitplaneIndex & 1) ? _bitplaneRegisters->bpl2mod : _bitplaneRegisters->bpl1mod);
}

void BitplaneDMA::DMAReadCallback(const ChipBusTimestamp &currentTime, UWO value)
{
  unsigned int bitplaneIndex = FetchProgramRunner.GetBitplaneIndex();

  _bitplaneRegisters->SetBplDat(bitplaneIndex, value);
  _bitplaneRegisters->AddBplPt(bitplaneIndex, 2);

  if (IsLastFetch && FetchProgramRunner.GetCanAddModulo())
  {
    _bitplaneRegisters->AddBplPt(bitplaneIndex, GetModuloForBitplane(bitplaneIndex));
  }

  FetchProgramRunner.NextStep();

  if (FetchProgramRunner.IsEnded())
  {
    if (!IsLastFetch && _ddfStateMachine->IsFetchEnabled())
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
    _bitplaneRegisters->AddBplPt(bitplaneIndex, GetModuloForBitplane(bitplaneIndex));
  }

  FetchProgramRunner.NextStep();

  if (FetchProgramRunner.IsEnded())
  {
    if (!IsLastFetch && _ddfStateMachine->IsFetchEnabled())
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
  _scheduler->InsertEvent(&bitplaneDMAEvent);
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
    dma_controller.ScheduleBitplaneDMA(fetchTime, _bitplaneRegisters->bplpt[bitplaneIndex], bitplaneIndex);
  }
}

bool BitplaneDMA::CanFetch()
{
  return _bitplaneRegisters->IsBitplaneDMAEnabled() && _bitplaneRegisters->EnabledBitplaneCount > 0 && diwy_state_machine.IsVisible() &&
         _ddfStateMachine->IsFetchEnabled();
}

bool BitplaneDMA::CalculateIsLastFetch(ULO atCycle)
{
  // Either ddf says this is the last one because stop has been seen
  if (_ddfStateMachine->IsNextFetchLast(atCycle))
  {
    return true;
  }

  // Or this is OCS and we ran into the hard stop that ends the line
  return (!chipsetGetECS() && (atCycle >= (chipsetGetMaximumDDF() - 4U)));
}

void BitplaneDMA::HandleEvent()
{
  bitplaneDMAEvent.Disable();
  InhibitedReadCallback(_scheduler->GetChipBusTimestamp());
}

void BitplaneDMA::StartFetchProgram(const ChipBusTimestamp &startTime)
{
  if (CanFetch())
  {
    IsLastFetch = CalculateIsLastFetch(startTime.GetCycle());
    FetchProgramRunner.StartProgram(FetchPrograms.GetFetchProgram(_bitplaneRegisters->IsLores, _bitplaneRegisters->EnabledBitplaneCount));

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

BitplaneDMA::BitplaneDMA(Scheduler *scheduler, BitplaneRegisters *bitplaneRegisters, DDFStateMachine *ddfStateMachine)
  : _scheduler(scheduler), _bitplaneRegisters(bitplaneRegisters), _ddfStateMachine(ddfStateMachine)
{
}
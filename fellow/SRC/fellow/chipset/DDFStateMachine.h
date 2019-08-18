#pragma once

#include "fellow/api/defs.h"
#include "fellow/time/Timestamps.h"

enum class DDFStates
{
  DDF_WAIT_FOR_START = 0,
  DDF_WAIT_FOR_STOP = 1
};

class DDFStateMachine
{
private:
  static const STR *DDFStateNames[2];

  bool _startFetchOncePerLine = true;
  ULO _previousFetchLine = 0;

  DDFStates _state = DDFStates::DDF_WAIT_FOR_START;

  bool _fetchEnabled = false;
  bool _endSeen = false;

  ULO _firstFetchStart = 0x18;
  ULO _lastFetchStart = 0x18;

  void SetState(DDFStates newState, ULO baseClockTimestamp);

  void EvaluateStateWaitForStart(const ChipBusTimestamp &currentTime);
  void EvaluateStateWaitForStop(const ChipBusTimestamp &currentTime);
  void EvaluateState(const ChipBusTimestamp &currentTime);

  void InitializeForNewFrame();
  void Reset();

public:
  bool IsNextFetchLast(ULO cycle280ns) const;
  bool IsFetchEnabled() const;

  void ChangedValue();

  static void HandleEvent();
  void Handler();

  void SoftReset();
  void HardReset();
  void EndOfFrame();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  DDFStateMachine() = default;
  ~DDFStateMachine() = default;
};

extern DDFStateMachine ddf_state_machine;

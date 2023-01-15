#pragma once

#include <functional>

#include "fellow/api/defs.h"

class Scheduler;

/* Cia registers, index 0 is Cia A, index 1 is Cia B */

struct cia_state
{
  ULO ta;
  ULO tb;
  ULO ta_rem; /* Preserves remainder when downsizing from bus cycles */
  ULO tb_rem;
  ULO talatch;
  ULO tblatch;
  LON taleft;
  LON tbleft;
  ULO evalarm;
  ULO evlatch;
  bool evlatching;
  ULO evwritelatch;
  bool evwritelatching;
  ULO ev;
  UBY icrreq;
  UBY icrmsk;
  UBY cra;
  UBY crb;
  UBY pra;
  UBY prb;
  UBY ddra;
  UBY ddrb;
  UBY sp;
};

class Cia
{
private:
  Scheduler *_scheduler{};
  cia_state _cia[2]{};

  std::function<UBY(ULO ciaIndex)> _read[16]{};
  std::function<void(ULO ciaIndex, UBY data)> _write[16]{};

  bool _recheck_irq{};
  ULO _recheck_irq_time{};
  ULO _next_event_type{};

  ULO GetBusCycleRatio();
  bool IsSoundFilterEnabled();
  bool IsPowerLEDEnabled();
  ULO UnstabilizeValue(ULO value, ULO remainder);
  ULO StabilizeValue(ULO value);
  ULO StabilizeValueRemainder(ULO value);
  ULO GetTimerValue(ULO value);
  void TAStabilize(ULO i);
  void TBStabilize(ULO i);
  void Stabilize(ULO i);
  void TAUnstabilize(ULO i);
  void TBUnstabilize(ULO i);
  void Unstabilize(ULO i);
  void UpdateIRQ(ULO i);
  void RaiseIRQ(ULO i, ULO req);
  void RaiseIndexIRQ();
  void CheckAlarmMatch(ULO i);
  void HandleTBTimeout(ULO i);
  void HandleTATimeout(ULO i);
  void UpdateEventCounter(ULO i);
  void UpdateTimersEOF();
  void EventSetup();
  void SetupNextEvent();
  void HandleEvent();
  void RecheckIRQ();
  UBY ReadApra();
  UBY ReadBpra();
  UBY Readpra(ULO i);
  void WriteApra(UBY data);
  void WriteBpra(UBY data);
  void Writepra(ULO i, UBY data);
  UBY Readprb(ULO i);
  void WriteAprb(UBY data);
  void WriteBprb(UBY data);
  void Writeprb(ULO i, UBY data);
  UBY Readddra(ULO i);
  void Writeddra(ULO i, UBY data);
  UBY Readddrb(ULO i);
  void Writeddrb(ULO i, UBY data);
  UBY Readsp(ULO i);
  void Writesp(ULO i, UBY data);
  UBY Readtalo(ULO i);
  UBY Readtahi(ULO i);
  void Writetalo(ULO i, UBY data);
  bool MustReloadOnTHiWrite(UBY cr);
  void Writetahi(ULO i, UBY data);
  UBY Readtblo(ULO i);
  UBY Readtbhi(ULO i);
  void Writetblo(ULO i, UBY data);
  void Writetbhi(ULO i, UBY data);
  UBY Readevlo(ULO i);
  UBY Readevmi(ULO i);
  UBY Readevhi(ULO i);
  void Writeevlo(ULO i, UBY data);
  void Writeevmi(ULO i, UBY data);
  void Writeevhi(ULO i, UBY data);
  UBY Readicr(ULO i);
  void Writeicr(ULO i, UBY data);
  UBY Readcra(ULO i);
  void Writecra(ULO i, UBY data);
  UBY Readcrb(ULO i);
  void Writecrb(ULO i, UBY data);
  UBY ReadNothing(ULO i);
  void WriteNothing(ULO i, UBY data);

  void StateClear();
  void InitializeInternalReadWriteFunctions();

public:
  UBY ReadByte(ULO address);
  void WriteByte(UBY data, ULO address);

  void StartEmulation();
  void StopEmulation();
  void HardReset();
  void Startup();
  void Shutdown();

  Cia(Scheduler *scheduler);
};
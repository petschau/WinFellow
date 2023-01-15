#pragma once

#include "fellow/api/defs.h"
#include "fellow/chipset/ChipsetCallbacks.h"
#include "fellow/memory/ChipsetBankHandler.h"

class Scheduler;

class BitplaneRegisters
{
private:
  Scheduler *_scheduler;
  IBitplaneShifter_AddDataCallback *BitplaneShifter_AddData;
  IBitplaneShifter_FlushCallback *BitplaneShifter_Flush;
  IColorChangeEventHandler *_colorChangeEventHandler;

  void ResetDiwhigh();
  void FlushShifter() const;

public:
  // Unmodified register values
  UWO lof;
  UWO diwstrt;
  UWO diwstop;
  UWO ddfstrt;
  UWO ddfstop;
  ULO bplpt[6];
  UWO bplcon0;
  UWO bplcon1;
  UWO bplcon2;
  UWO bplcon3; // ECS
  UWO bpl1mod;
  UWO bpl2mod;
  UWO bpldat[6];
  UWO color[64];
  UWO htotal;   // ECS
  UWO hsstop;   // ECS
  UWO hbstrt;   // ECS
  UWO hbstop;   // ECS
  UWO vtotal;   // ECS
  UWO vsstop;   // ECS
  UWO vbstrt;   // ECS
  UWO vbstop;   // ECS
  UWO beamcon0; // ECS
  UWO hsstrt;   // ECS
  UWO vsstrt;   // ECS
  UWO hcenter;  // ECS
  UWO diwhigh;  // ECS

  // Inferred values
  // Decoded from bplcon0
  bool IsLores;
  bool IsHires;
  bool IsDualPlayfield;
  bool IsHam;
  ULO EnabledBitplaneCount;

  // Decoded from OCS DIW and possibly DIWHIGH if using ECS and DIWHIGH was written
  UWO DiwXStart;
  UWO DiwXStop;
  UWO DiwYStart;
  UWO DiwYStop;

  // Decoded from ECS Beamcon0
  bool HardBlankDisable;
  bool IgnoreLatchedPenValueOnVerticalPosRead;
  bool HardVerticalWindowDisable;
  bool LongLineToggleDisable;
  bool CompositeSyncRedirection;
  bool VariableVerticalSyncEnable;
  bool VariableHorisontalSyncEnable;
  bool VariableBeamCounterComparatorEnable;
  bool SpecialUltraResolutionModeEnable;
  bool ProgrammablePALModeEnable;
  bool VariableCompositeSyncEnable;
  bool CompositeBlankRedirectionEnable;
  bool CSyncPinTrue;
  bool VSyncPinTrue;
  bool HSyncPinTrue;

  static UWO GetDeniseId();
  static UWO GetAgnusId();
  UWO GetVerticalPosition();
  UWO GetHorisontalPosition();

  void PublishColorChanged(const unsigned int colorIndex, const UWO color12Bit, const UWO halfbriteColor12Bit);

  UWO GetVPosR() const;
  UWO GetVHPosR();
  void SetVPos(UWO data);
  void SetVHPos(UWO data);
  void SetDIWStrt(UWO data);
  void SetDIWStop(UWO data);
  void SetDDFStrt(UWO data);
  void SetDDFStop(UWO data);
  void SetBplPtHigh(unsigned int index, UWO data);
  void SetBplPtLow(unsigned int index, UWO data);
  void SetBplCon0(UWO data);
  void SetBplCon1(UWO data);
  void SetBplCon2(UWO data);
  void SetBpl1Mod(UWO data);
  void SetBpl2Mod(UWO data);
  void SetBplDat(unsigned int index, UWO data);
  void SetColor(unsigned int index, UWO data);
  void SetHTotal(UWO data);
  void SetHCenter(UWO data);
  void SetHBStrt(UWO data);
  void SetHBStop(UWO data);
  void SetHSStrt(UWO data);
  void SetHSStop(UWO data);
  void SetVTotal(UWO data);
  void SetVBStrt(UWO data);
  void SetVBStop(UWO data);
  void SetVSStrt(UWO data);
  void SetVSStop(UWO data);
  void SetBeamcon0(UWO data);
  void SetDIWHigh(UWO data);

  void InferBeamcon0Values();

  void AddBplPt(unsigned int index, ULO add);
  void LinearAddModulo(unsigned int enabledBitplaneCount, ULO bplLengthInBytes);
  void AddModulo();

  void SetAddDataCallback(IBitplaneShifter_AddDataCallback *bitplaneShifter_AddData);
  void SetFlushCallback(IBitplaneShifter_FlushCallback *bitplaneShifter_Flush);
  void SetColorChangeEventHandler(IColorChangeEventHandler *colorChangeEventHandler);

  void InstallIOHandlers(ChipsetBankHandler &chipsetBankHandler, bool isECS);

  void ClearState();

  void Initialize(Scheduler *scheduler);

  BitplaneRegisters();
  ~BitplaneRegisters();
};

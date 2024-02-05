#pragma once

#include "Driver/Drivers.h"
#include "Module/Hardfile/IHardfileHandler.h"
#include "Service/ILog.h"
#include "Service/IFileops.h"
#include "Service/IFileInformation.h"
#include "Service/IHud.h"
#include "Service/IRetroPlatform.h"
#include "CustomChipset/Sound/Sound.h"
#include "CustomChipset/Registers.h"
#include "CustomChipset/RegisterUtility.h"
#include "IO/Uart.h"
#include "IO/RtcOkiMsm6242rs.h"
#include "Scheduler/Scheduler.h"
#include "Scheduler/Clocks.h"
#include "Cpu/ICpu.h"
#include "CustomChipset/IAgnus.h"
#include "CustomChipset/Copper/ICopper.h"
#include "CustomChipset/Copper/CopperRegisters.h"
#include "IO/ICia.h"
#include "CustomChipset/IBlitter.h"
#include "CustomChipset/IPaula.h"
#include "CustomChipset/Sprite/SpriteRegisters.h"
#include "CustomChipset/Sprite/ISprites.h"
#include "CustomChipset/Sprite/LineExactSprites.h"
#include "CustomChipset/Sprite/CycleExactSprites.h"
#include "Memory/IMemory.h"
#include "DebugApi/DebugVM.h"
#include "DebugApi/DebugLog.h"

class Core
{
private:
  void ConfigureForCycleAccuracy();
  void ConfigureForLineAccuracy();

public:
  Registers Registers;
  CopperRegisters *CopperRegisters;
  SpriteRegisters *SpriteRegisters;
  RegisterUtility RegisterUtility;

  FrameParameters *CurrentFrameParameters;
  Sound *Sound;
  Uart *Uart;
  RtcOkiMsm6242rs *RtcOkiMsm6242rs;
  Clocks *Clocks;
  SchedulerEvents *Events;
  Scheduler *Scheduler;
  ICpu *Cpu;
  IAgnus *Agnus;
  ICia *Cia;
  IMemory *Memory;

  ICopper *LineExactCopper;
  ICopper *CycleExactCopper;
  ICopper *CurrentCopper;

  LineExactSprites *LineExactSprites;
  CycleExactSprites *CycleExactSprites;
  ISprites *CurrentSprites;

  IBlitter *Blitter;
  IPaula *Paula;

  Module::Hardfile::IHardfileHandler *HardfileHandler;

  Drivers Drivers;
  DebugLog *DebugLog;
  Service::ILog *Log;
  Service::IFileops *Fileops;
  Service::IFileInformation *FileInformation;
  Service::IHud *Hud;
  Service::IRetroPlatform *RP;

  Debug::DebugVM DebugVM;

  void ConfigureAccuracy(bool cycleAccuracy);

  Core();
  ~Core();
};

extern Core _core;

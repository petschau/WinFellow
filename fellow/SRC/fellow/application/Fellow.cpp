/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* This file has overall control over starting and stopping Fellow         */
/* Everything starts here                                                  */
/*                                                                         */
/* Author: Petter Schau                                                    */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

#include "fellow/api/defs.h"
#include "versioninfo.h"
#include "fellow/api/Drivers.h"
#include "fellow/os/windows/application/DriversFactory.h"
#include "fellow/application/Fellow.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/cpu/CpuModule.h"
#include "fellow/cpu/CpuIntegration.h"
#include "fellow/memory/Memory.h"
#include "fellow/chipset/Floppy.h"
#include "fellow/chipset/Sound.h"
#include "fellow/application/Gameport.h"
#include "fellow/chipset/Keyboard.h"
#include "fellow/chipset/Graphics.h"
#include "fellow/api/modules/IHardfileHandler.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/chipset/CopperCommon.h"
#include "fellow/chipset/Cia.h"
#include "fellow/chipset/Blitter.h"
#include "fellow/chipset/Sprite.h"
#include "fellow/configuration/Configuration.h"
#include "fellow/application/FellowFilesys.h"
#include "fellow/os/windows/application/sysinfo.h"
#include "fellow/application/Interrupt.h"
#include "fellow/chipset/uart.h"
#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif
#include "fellow/chipset/DDFStateMachine.h"
#include "fellow/chipset/DIWXStateMachine.h"
#include "fellow/chipset/DIWYStateMachine.h"
#include "fellow/chipset/DMAController.h"
#include "fellow/chipset/Planar2ChunkyDecoder.h"
#include "fellow/chipset/BitplaneShifter.h"
#include "fellow/chipset/HostFrameDelayedRenderer.h"
#include "fellow/chipset/HostFrameCopier.h"

#include "fellow/automation/Automator.h"
#include "fellow/service/CoreServicesFactory.h"
#include "fellow/api/VM.h"

using namespace std;
using namespace fellow::api::modules;
using namespace fellow::api;
using namespace fellow::service;

void Fellow::LogPerformanceCounterPercentage(const IPerformanceCounter *counter, LLO ticksPerFrame, ULL frameCount)
{
  float callsPerFrame = (float)counter->GetCallCount() / (float)frameCount;
  float timePerFrame = (float)counter->GetAverage() * callsPerFrame;
  float percentagePerFrame = 100.0f * timePerFrame / (float)ticksPerFrame;

  Service->Log.AddLog("\t%s percentage per frame: %f\n", counter->GetName(), percentagePerFrame);
}

void Fellow::LogPerformanceReport()
{
  ULL frameCount = _scheduler.GetRasterFrameCount();
  LLO totalRuntime = _emulationRunPerformanceCounter->GetTotalTime();
  if (frameCount == 0 || totalRuntime == 0)
  {
    Service->Log.AddLog("Performance report: Not available, no emulation sessions run\n");
    return;
  }

  LLO frequency = _emulationRunPerformanceCounter->GetFrequency();
  LLO ticksPerFrame = totalRuntime / frameCount;
  LLO ticksPer50Frames = (totalRuntime * 50) / frameCount;
  float framesPerSecond = ((float)frequency) / (float)ticksPerFrame;

  Service->Log.AddLog("Performance report:\n");
  Service->Log.AddLog("\tFrame count:         %llu\n", frameCount);
  Service->Log.AddLog("\tTotal runtime ticks: %lld\n", totalRuntime);
  Service->Log.AddLog("\tTicks per frame:     %lld\n", ticksPerFrame);
  Service->Log.AddLog("\tTicks per 50 frames: %lld\n", ticksPer50Frames);
  Service->Log.AddLog("\tFrames per second:   %f\n", framesPerSecond);

  LogPerformanceCounterPercentage(bitplane_shifter.PerformanceCounter, ticksPerFrame, frameCount);
  LogPerformanceCounterPercentage(host_frame_delayed_renderer.PerformanceCounter, ticksPerFrame, frameCount);
  LogPerformanceCounterPercentage(host_frame_copier.PerformanceCounter, ticksPerFrame, frameCount);
}

void Fellow::RequestResetBeforeStartingEmulation()
{
  _performResetBeforeStartingEmulation = true;
}

//===============
// setjmp support
//===============

void Fellow::SetRuntimeErrorCode(fellow_runtime_error_codes error_code)
{
  _runtimeErrorCode = error_code;
}

fellow_runtime_error_codes Fellow::GetRuntimeErrorCode()
{
  return _runtimeErrorCode;
}

string Fellow::GetVersionString()
{
  ostringstream versionstring;

#ifdef X64
  unsigned int bits = 64;
#else
  unsigned int bits = 32;
#endif

  versionstring << FELLOWVERSION << " - " << bits;

  return versionstring.str();
}

void Fellow::HandleCpuPcBadBank()
{
  const char *errorMessage = "A serious emulation runtime error occured:\nThe emulated CPU entered Amiga memory that can not hold\nexecutable data. "
                             "Emulation could not continue.";
  Service->Log.AddLog(errorMessage);
  Driver->Gui->Requester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, errorMessage);
}

void Fellow::RuntimeErrorCheck()
{
  switch (GetRuntimeErrorCode())
  {
    case fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK: HandleCpuPcBadBank(); break;
    case fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR: break;
  }
  SetRuntimeErrorCode(fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR);
}

void Fellow::SoftReset()
{
  _scheduler.SoftReset();
  if (chipsetIsCycleExact())
  {
    dma_controller.SoftReset();
    ddf_state_machine.SoftReset();
    diwx_state_machine.SoftReset();
    diwy_state_machine.SoftReset();
    bitplane_shifter.SoftReset();
  }
  memorySoftReset();
  interruptSoftReset();
  _modules.HardfileHandler->HardReset();
  spriteHardReset();
  Draw.HardReset();
  _modules.Keyboard->HardReset();
  gameportHardReset();
  soundHardReset();
  blitterHardReset();
  copperHardReset();
  floppyHardReset();
  ciaHardReset();
  graphHardReset();
  ffilesysHardReset();
  memoryHardResetPost();
  _performResetBeforeStartingEmulation = false;
}

void Fellow::HardReset()
{
  _scheduler.HardReset();
  if (chipsetIsCycleExact())
  {
    dma_controller.HardReset();
    ddf_state_machine.HardReset();
    diwx_state_machine.HardReset();
    diwy_state_machine.HardReset();
    _modules.BitplaneRegisters->ClearState();
    bitplane_shifter.HardReset();
  }
  memoryHardReset();
  interruptHardReset();
  _modules.HardfileHandler->HardReset();
  spriteHardReset();
  Draw.HardReset();
  _modules.Keyboard->HardReset();
  gameportHardReset();
  soundHardReset();
  blitterHardReset();
  copperHardReset();
  floppyHardReset();
  ciaHardReset();
  graphHardReset();
  ffilesysHardReset();
  memoryHardResetPost();
  cpuIntegrationHardReset();
  _performResetBeforeStartingEmulation = false;
}

//=======================================================
// Modules use this to request that emulation stop safely
// Emulation will stop at the beginning of the next frame
//=======================================================

void Fellow::RequestEmulationStop()
{
  _scheduler.RequestEmulationStop();
}

bool Fellow::StartEmulation()
{
  Driver->Ini->EmulationStart();
  memoryEmulationStart();
  interruptEmulationStart();
  ciaEmulationStart();
  cpuIntegrationEmulationStart();
  spriteEmulationStart();
  blitterEmulationStart();
  copperEmulationStart();

  if (!Draw.EmulationStart())
  {
    return false;
  }

  _modules.Keyboard->StartEmulation();
  gameportEmulationStart();

  if (!Draw.EmulationStartPost())
  {
    return false;
  }

  graphEmulationStart();
  soundEmulationStart();
  _scheduler.EmulationStart();
  floppyEmulationStart();
  ffilesysEmulationStart();
  Driver->Timer->EmulationStart();

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.EmulationStart();
#endif

  if (chipsetIsCycleExact())
  {
    ddf_state_machine.EmulationStart();
    diwx_state_machine.EmulationStart();
    diwy_state_machine.EmulationStart();
    dma_controller.EmulationStart();
    bitplane_shifter.EmulationStart();
  }

  uart.EmulationStart();
  _modules.HardfileHandler->EmulationStart();

  _emulationRunPerformanceCounter->Start();

  bool kickstartOk = memoryGetKickImageOK();

  if (_performResetBeforeStartingEmulation)
  {
    HardReset();
  }

  return kickstartOk;
}

void Fellow::StopEmulation()
{
  _emulationRunPerformanceCounter->Stop();

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.EmulationStop();
#endif
  _modules.HardfileHandler->EmulationStop();
  Driver->Timer->EmulationStop();
  ffilesysEmulationStop();
  floppyEmulationStop();
  _scheduler.EmulationStop();
  soundEmulationStop();
  gameportEmulationStop();
  _modules.Keyboard->StopEmulation();
  Draw.EmulationStop();
  copperEmulationStop();
  blitterEmulationStop();
  spriteEmulationStop();
  graphEmulationStop();
  cpuIntegrationEmulationStop();
  ciaEmulationStop();
  interruptEmulationStop();
  memoryEmulationStop();
  Driver->Ini->EmulationStop();
  if (chipsetIsCycleExact())
  {
    ddf_state_machine.EmulationStop();
    diwx_state_machine.EmulationStop();
    diwy_state_machine.EmulationStop();
    dma_controller.EmulationStop();
    bitplane_shifter.EmulationStop();
  }

  uart.EmulationStop();
}

void Fellow::Run()
{
  SetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(_runtimeErrorEnvironment));
  if (GetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    _scheduler.Run();
  }

  RuntimeErrorCheck();
}

//==========================
// Steps one CPU instruction
//==========================

void Fellow::StepOne()
{
  SetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(_runtimeErrorEnvironment));
  if (GetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    _scheduler.DebugStepOneInstruction();
  }
  RuntimeErrorCheck();
}

//=============================
// Steps over a CPU instruction
//=============================

void Fellow::StepOver()
{
  SetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(_runtimeErrorEnvironment));
  if (GetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    char saddress[128];
    char sdata[128];
    char sinstruction[128];
    char soperands[128];
    ULO current_pc = cpuGetPC();
    ULO over_pc = cpuDisOpcode(current_pc, saddress, sdata, sinstruction, soperands);
    bool emulationStopRequested = false;

    while ((cpuGetPC() != over_pc) && !emulationStopRequested)
    {
      emulationStopRequested = _scheduler.DebugStepOneInstruction();
    }
  }

  RuntimeErrorCheck();
}

//==============================================
// Run until we crash or is exited in debug-mode
//==============================================

void Fellow::RunDebug(ULO breakpoint)
{
  SetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(_runtimeErrorEnvironment));

  if (GetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    bool emulationStopRequested = false;

    while ((!emulationStopRequested) && (breakpoint != cpuGetPC()))
    {
      emulationStopRequested = _scheduler.DebugStepOneInstruction();
    }
  }

  RuntimeErrorCheck();
}

//=============================================
// Run until we're past the current raster line
//=============================================

void Fellow::StepLine()
{
  SetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(_runtimeErrorEnvironment));

  if (GetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    ULL currentFrame = _scheduler.GetRasterFrameCount();
    ULO currentLine = _scheduler.GetRasterY();
    bool emulationStopRequested = false;

    while ((!emulationStopRequested) && currentLine == _scheduler.GetRasterY() && currentFrame == _scheduler.GetRasterFrameCount())
    {
      emulationStopRequested = _scheduler.DebugStepOneInstruction();
    }
  }

  RuntimeErrorCheck();
}

//=======================================
// Run until we're past the current frame
//=======================================

void Fellow::StepFrame()
{
  SetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(_runtimeErrorEnvironment));

  if (GetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    ULL currentFrame = _scheduler.GetRasterFrameCount();
    bool emulationStopRequested = false;

    while ((!emulationStopRequested) && currentFrame == _scheduler.GetRasterFrameCount())
    {
      emulationStopRequested = _scheduler.DebugStepOneInstruction();
    }
  }

  RuntimeErrorCheck();
}

//========================================================
// Something really bad happened, immediate emulation stop
//========================================================

void Fellow::NastyExit()
{
  longjmp(_runtimeErrorEnvironment, (int)GetRuntimeErrorCode());
  fprintf(stderr, "You only die twice, I give in!\n");
  soundShutdown();
  fprintf(stderr, "Serious error! Exit.\n");
  exit(EXIT_FAILURE);
}

//========================================
// Draw subsystem failure message and exit
//========================================

void Fellow::DrawFailed()
{
  const char *errorMessage = "Graphics subsystem failed to start.\nPlease check your OS graphics driver setup.\nClosing down application.";
  Service->Log.AddLog(errorMessage);
  Driver->Gui->Requester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, errorMessage);
  exit(EXIT_FAILURE);
}

void Fellow::CreateModules()
{
  _emulationRunPerformanceCounter = Service->PerformanceCounterFactory.Create("Fellow emulation runtime");

  _modules.HardfileHandler = CreateHardfileHandler(_vm);
  _modules.Keyboard = new Keyboard(this);
  _modules.BitplaneRegisters = new BitplaneRegisters();
}

void Fellow::InitializeModules(int argc, char *argv[])
{
  RequestResetBeforeStartingEmulation();

  chipsetStartup();
  _modules.HardfileHandler->Startup();
  ffilesysStartup();
  spriteStartup();
  _modules.Keyboard->Startup();
  cfgStartup(argc, argv);
  if (!Draw.Startup())
  {
    DrawFailed();
  }
  gameportStartup();
  _scheduler.Startup(&_modules);
  _modules.BitplaneRegisters->Initialize(&_scheduler);
  soundStartup();
  blitterStartup();
  copperStartup();
  floppyStartup();
  ciaStartup();
  memoryStartup();
  interruptStartup();
  graphStartup();
  cpuIntegrationStartup();

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.Startup();
#endif

  ddf_state_machine.Startup();
  diwx_state_machine.Startup();
  diwy_state_machine.Startup();
  dma_controller.Startup();
  Planar2ChunkyDecoder::Startup();
  bitplane_shifter.Startup();
  host_frame_copier.Startup();
  host_frame_delayed_renderer.Startup();
  uart.Startup();
  automator.Startup();

  _emulationRunPerformanceCounter->LogTimerProperties();
}

void Fellow::DeleteModules()
{
  LogPerformanceReport();

  automator.Shutdown();
  uart.Shutdown();
  bitplane_shifter.Shutdown();
  Planar2ChunkyDecoder::Shutdown();
  dma_controller.Shutdown();
  diwy_state_machine.Shutdown();
  diwx_state_machine.Shutdown();
  ddf_state_machine.Shutdown();

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.Shutdown();
#endif
  cpuIntegrationShutdown();
  graphShutdown();
  interruptShutdown();
  memoryShutdown();
  ciaShutdown();
  floppyShutdown();
  copperShutdown();
  blitterShutdown();
  soundShutdown();

  delete _modules.BitplaneRegisters;

  _scheduler.Shutdown();

  gameportShutdown();
  Draw.Shutdown();
  cfgShutdown();

  _modules.Keyboard->Shutdown();
  delete _modules.Keyboard;

  spriteShutdown();
  ffilesysShutdown();

  _modules.HardfileHandler->Shutdown();
  delete _modules.HardfileHandler;

  delete _emulationRunPerformanceCounter;
  _emulationRunPerformanceCounter = nullptr;
}

void Fellow::CreateDrivers()
{
  Driver = DriversFactory::Create();
}

void Fellow::InitializeDrivers()
{
  Driver->Timer->Initialize();
  Driver->Ini->Initialize();
  Driver->Graphics->Initialize(this);
  Driver->Gui->Initialize(this);
  Driver->Keyboard->Initialize(_modules.Keyboard);
}

void Fellow::DeleteDrivers()
{
  Driver->Keyboard->Release();
  Driver->Gui->Release();
  Driver->Graphics->Release();
  Driver->Ini->Release();
  Driver->Timer->Release();

  DriversFactory::Delete(Driver);
  Driver = nullptr;
}

void Fellow::CreateServices()
{
  Service = CoreServicesFactory::Create();
}

void Fellow::DeleteServices()
{
  CoreServicesFactory::Delete(Service);
  Service = nullptr;
}

void Fellow::CreateVM()
{
  _vm = CreateVirtualMachine(this);
}

void Fellow::DeleteVM()
{
  delete _vm;
}

VirtualMachine *Fellow::GetVM()
{
  return _vm;
}

void Fellow::RunApplication()
{
#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
#endif
    // set DPI awareness in standalone GUI mode to system DPI aware
    Driver->Gui->SetProcessDPIAwareness("2");
    Driver->Gui->BeforeEnter();
    while (!Driver->Gui->Enter())
    {
      Run();
    }
#ifdef RETRO_PLATFORM
  }
  else
    RP.EnterHeadlessMode(this);
#endif
}

Fellow::Fellow(int argc, char *argv[])
{
  CreateServices();
  sysinfoLogSysInfo(this);
  CreateVM();
  CreateDrivers();
  CreateModules();

  InitializeDrivers();
  InitializeModules(argc, argv);
}

Fellow::~Fellow()
{
  DeleteModules();
  DeleteDrivers();
  DeleteVM();
  DeleteServices();
}

int __cdecl main(int argc, char *argv[])
{
  auto fellow = new Fellow(argc, argv);
  fellow->RunApplication();
  delete fellow;

  return EXIT_SUCCESS;
}

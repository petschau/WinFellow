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
#include "fellow/api/Services.h"
#include "fellow/application/Fellow.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/cpu/CpuModule.h"
#include "fellow/cpu/CpuIntegration.h"
#include "fellow/memory/Memory.h"
#include "fellow/chipset/Floppy.h"
#include "fellow/chipset/Sound.h"
#include "fellow/application/Gameport.h"
#include "fellow/chipset/Kbd.h"
#include "fellow/chipset/Graphics.h"
#include "fellow/api/modules/IHardfileHandler.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/chipset/CopperCommon.h"
#include "fellow/chipset/Cia.h"
#include "fellow/chipset/Blitter.h"
#include "fellow/chipset/Sprite.h"
#include "fellow/configuration/Configuration.h"
#include "fellow/application/FellowFilesys.h"
#include "fellow/application/Ini.h"
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

BOOLE fellow_request_emulation_stop;

IPerformanceCounter *fellow_emulation_run_performance_counter = nullptr;

void fellowLogPerformanceCounterPercentage(const IPerformanceCounter *counter, LLO ticksPerFrame, ULL frameCount)
{
  float callsPerFrame = (float)counter->GetCallCount() / (float)frameCount;
  float timePerFrame = (float)counter->GetAverage() * callsPerFrame;
  float percentagePerFrame = 100.0f * timePerFrame / (float)ticksPerFrame;

  Service->Log.AddLog("\t%s percentage per frame: %f\n", counter->GetName(), percentagePerFrame);
}

void fellowLogPerformanceReport()
{
  ULL frameCount = scheduler.GetRasterFrameCount();
  LLO totalRuntime = fellow_emulation_run_performance_counter->GetTotalTime();
  if (frameCount != 0 && totalRuntime != 0)
  {
    LLO frequency = fellow_emulation_run_performance_counter->GetFrequency();
    LLO ticksPerFrame = totalRuntime / frameCount;
    LLO ticksPer50Frames = (totalRuntime * 50) / frameCount;
    float framesPerSecond = ((float)frequency) / (float)ticksPerFrame;

    Service->Log.AddLog("Performance report:\n");
    Service->Log.AddLog("\tFrame count:         %llu\n", frameCount);
    Service->Log.AddLog("\tTotal runtime ticks: %lld\n", totalRuntime);
    Service->Log.AddLog("\tTicks per frame:     %lld\n", ticksPerFrame);
    Service->Log.AddLog("\tTicks per 50 frames: %lld\n", ticksPer50Frames);
    Service->Log.AddLog("\tFrames per second:   %f\n", framesPerSecond);

    fellowLogPerformanceCounterPercentage(bitplane_shifter.PerformanceCounter, ticksPerFrame, frameCount);
    fellowLogPerformanceCounterPercentage(host_frame_delayed_renderer.PerformanceCounter, ticksPerFrame, frameCount);
    fellowLogPerformanceCounterPercentage(host_frame_copier.PerformanceCounter, ticksPerFrame, frameCount);
  }
  else
  {
    Service->Log.AddLog("Performance report: Not available, no emulation sessions run\n");
  }
}

/*============================================================================*/
/* Perform reset before starting emulation flag                               */
/*============================================================================*/

bool fellow_pre_start_reset;

void fellowSetPreStartReset(bool reset)
{
  fellow_pre_start_reset = reset;
}

bool fellowGetPreStartReset()
{
  return fellow_pre_start_reset;
}

/*============================================================================*/
/* setjmp support                                                             */
/*============================================================================*/

static jmp_buf fellow_runtime_error_env;
static fellow_runtime_error_codes fellow_runtime_error_code;

void fellowSetRuntimeErrorCode(fellow_runtime_error_codes error_code)
{
  fellow_runtime_error_code = error_code;
}

static fellow_runtime_error_codes fellowGetRuntimeErrorCode()
{
  return fellow_runtime_error_code;
}

string fellowGetVersionString()
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

/*============================================================================*/
/* Runtime Error Check                                                        */
/*============================================================================*/

void fellowHandleCpuPcBadBank()
{
  const char *errorMessage = "A serious emulation runtime error occured:\nThe emulated CPU entered Amiga memory that can not hold\nexecutable data. Emulation could not continue.";
  Service->Log.AddLog(errorMessage);
  Driver->Gui.Requester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, errorMessage);
}

static void fellowRuntimeErrorCheck()
{
  switch (fellowGetRuntimeErrorCode())
  {
    case fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK: fellowHandleCpuPcBadBank(); break;
    case fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR: break;
  }
  fellowSetRuntimeErrorCode(fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR);
}

/*============================================================================*/
/* Softreset                                                                  */
/*============================================================================*/

void fellowSoftReset()
{
  scheduler.SoftReset();
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
  HardfileHandler->HardReset();
  spriteHardReset();
  Draw.HardReset();
  kbdHardReset();
  gameportHardReset();
  soundHardReset();
  blitterHardReset();
  copperHardReset();
  floppyHardReset();
  ciaHardReset();
  graphHardReset();
  ffilesysHardReset();
  memoryHardResetPost();
  fellowSetPreStartReset(false);
}

/*============================================================================*/
/* Hardreset                                                                  */
/*============================================================================*/

void fellowHardReset()
{
  scheduler.HardReset();
  if (chipsetIsCycleExact())
  {
    dma_controller.HardReset();
    ddf_state_machine.HardReset();
    diwx_state_machine.HardReset();
    diwy_state_machine.HardReset();
    bitplane_registers.ClearState();
    bitplane_shifter.HardReset();
  }
  memoryHardReset();
  interruptHardReset();
  HardfileHandler->HardReset();
  spriteHardReset();
  Draw.HardReset();
  kbdHardReset();
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
  fellowSetPreStartReset(false);
}

/*============================================================================*/
/* Modules use this to request that emulation stop safely.                    */
/* Emulation will stop at the beginning of the next frame                     */
/*============================================================================*/

void fellowRequestEmulationStop()
{
  fellow_request_emulation_stop = TRUE;
}

void fellowRequestEmulationStopClear()
{
  fellow_request_emulation_stop = FALSE;
}

/*============================================================================*/
/* Controls the process of starting actual emulation                          */
/*============================================================================*/

bool fellowEmulationStart()
{
  fellowRequestEmulationStopClear();
  Driver->Ini.EmulationStart();
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

  kbdEmulationStart();
  gameportEmulationStart();

  if (!Draw.EmulationStartPost())
  {
    return false;
  }

  graphEmulationStart();
  soundEmulationStart();
  scheduler.EmulationStart();
  floppyEmulationStart();
  ffilesysEmulationStart();
  Driver->Timer.EmulationStart();

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
  HardfileHandler->EmulationStart();

  fellow_emulation_run_performance_counter->Start();

  return memoryGetKickImageOK();
}

/*============================================================================*/
/* Controls the process of halting actual emulation                           */
/*============================================================================*/

void fellowEmulationStop()
{
  fellow_emulation_run_performance_counter->Stop();

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.EmulationStop();
#endif
  HardfileHandler->EmulationStop();
  Driver->Timer.EmulationStop();
  ffilesysEmulationStop();
  floppyEmulationStop();
  scheduler.EmulationStop();
  soundEmulationStop();
  gameportEmulationStop();
  kbdEmulationStop();
  Draw.EmulationStop();
  copperEmulationStop();
  blitterEmulationStop();
  spriteEmulationStop();
  graphEmulationStop();
  cpuIntegrationEmulationStop();
  ciaEmulationStop();
  interruptEmulationStop();
  memoryEmulationStop();
  Driver->Ini.EmulationStop();
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

/*============================================================================*/
/* Starts emulation                                                           */
/* FellowEmulationStart() has already been called                             */
/* FellowEmulationStop() will be called elsewhere                             */
/*============================================================================*/

void fellowRun()
{
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  fellowSetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    scheduler.Run();
  }

  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

/*============================================================================*/
/* Steps one CPU instruction                                                  */
/*============================================================================*/

void fellowStepOne()
{
  fellowRequestEmulationStopClear();
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }
  fellowSetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    scheduler.DebugStepOneInstruction();
  }
  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

/*============================================================================*/
/* Steps over a CPU instruction                                               */
/*============================================================================*/

void fellowStepOver()
{
  fellowRequestEmulationStopClear();
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  fellowSetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    char saddress[128];
    char sdata[128];
    char sinstruction[128];
    char soperands[128];
    ULO current_pc = cpuGetPC();
    ULO over_pc = cpuDisOpcode(current_pc, saddress, sdata, sinstruction, soperands);
    while ((cpuGetPC() != over_pc) && !fellow_request_emulation_stop)
    {
      scheduler.DebugStepOneInstruction();
    }
  }

  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

/*============================================================================*/
/* Run until we crash or is exited in debug-mode                              */
/*============================================================================*/

void fellowRunDebug(ULO breakpoint)
{
  fellowRequestEmulationStopClear();
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  fellowSetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(fellow_runtime_error_env));

  if (fellowGetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    while ((!fellow_request_emulation_stop) && (breakpoint != cpuGetPC()))
    {
      scheduler.DebugStepOneInstruction();
    }
  }

  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

/*============================================================================*/
/* Run until we're past the current raster line                               */
/*============================================================================*/

void fellowStepLine()
{
  fellowRequestEmulationStopClear();
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  fellowSetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(fellow_runtime_error_env));

  if (fellowGetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    ULL currentFrame = scheduler.GetRasterFrameCount();
    ULO currentLine = scheduler.GetRasterY();
    while ((!fellow_request_emulation_stop) && currentLine == scheduler.GetRasterY() && currentFrame == scheduler.GetRasterFrameCount())
    {
      scheduler.DebugStepOneInstruction();
    }
  }

  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

/*============================================================================*/
/* Run until we're past the current frame                                     */
/*============================================================================*/

void fellowStepFrame()
{
  fellowRequestEmulationStopClear();
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  fellowSetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(fellow_runtime_error_env));

  if (fellowGetRuntimeErrorCode() == fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    ULL currentFrame = scheduler.GetRasterFrameCount();
    while ((!fellow_request_emulation_stop) && currentFrame == scheduler.GetRasterFrameCount())
    {
      scheduler.DebugStepOneInstruction();
    }
  }

  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

/*============================================================================*/
/* Something really bad happened, immediate emulation stop                    */
/*============================================================================*/

void fellowNastyExit()
{
  longjmp(fellow_runtime_error_env, (int)fellowGetRuntimeErrorCode());
  fprintf(stderr, "You only die twice, I give in!\n");
  soundShutdown();
  fprintf(stderr, "Serious error! Exit.\n");
  exit(EXIT_FAILURE);
}

/*============================================================================*/
/* Draw subsystem failure message and exit                                    */
/*============================================================================*/

static void fellowDrawFailed()
{
  const char *errorMessage = "Graphics subsystem failed to start.\nPlease check your OS graphics driver setup.\nClosing down application.";
  Service->Log.AddLog(errorMessage);
  Driver->Gui.Requester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, errorMessage);
  exit(EXIT_FAILURE);
}

/*============================================================================*/
/* Inititalize all modules in the emulator, called on startup                 */
/*============================================================================*/

static void fellowModulesStartup(int argc, char *argv[])
{
  fellow_emulation_run_performance_counter = Service->PerformanceCounterFactory.Create("Fellow emulation runtime");

  chipsetStartup();
  Driver->Timer.Startup();
  HardfileHandler->Startup();
  ffilesysStartup();
  spriteStartup();
  Driver->Ini.Startup();
  kbdStartup();
  cfgStartup(argc, argv);
  if (!Draw.Startup()) fellowDrawFailed();
  gameportStartup();
  scheduler.Startup();
  soundStartup();
  blitterStartup();
  copperStartup();
  floppyStartup();
  ciaStartup();
  memoryStartup();
  interruptStartup();
  graphStartup();
  cpuIntegrationStartup();
  Driver->Gui.Startup();
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

  fellow_emulation_run_performance_counter->LogTimerProperties();
}

/*============================================================================*/
/* Release all modules in the emulator, called on shutdown                    */
/*============================================================================*/

static void fellowModulesShutdown()
{
  fellowLogPerformanceReport();

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
  Driver->Gui.Shutdown();
  cpuIntegrationShutdown();
  graphShutdown();
  interruptShutdown();
  memoryShutdown();
  ciaShutdown();
  floppyShutdown();
  copperShutdown();
  blitterShutdown();
  soundShutdown();
  scheduler.Shutdown();
  gameportShutdown();
  Draw.Shutdown();
  cfgShutdown();
  kbdShutdown();
  Driver->Ini.Shutdown();
  spriteShutdown();
  ffilesysShutdown();
  HardfileHandler->Shutdown();
  Driver->Timer.Shutdown();

  delete HardfileHandler;

  CoreServicesFactory::Delete(Service);
  Service = nullptr;

  delete VM;

  delete fellow_emulation_run_performance_counter;
  fellow_emulation_run_performance_counter = nullptr;
}

void fellowCreateServices()
{
  Service = CoreServicesFactory::Create();
}

void fellowDeleteServices()
{
  CoreServicesFactory::Delete(Service);
  Service = nullptr;
}

/*============================================================================*/
/* main....                                                                   */
/*============================================================================*/

int main(int argc, char *argv[])
{
  fellowCreateServices();
  sysinfoLogSysInfo();
  fellowSetPreStartReset(true);
  fellowModulesStartup(argc, argv);

#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
#endif
    // set DPI awareness in standalone GUI mode to system DPI aware
    Driver->Gui.SetProcessDPIAwareness("2");
    while (!Driver->Gui.Enter())
      fellowRun();
#ifdef RETRO_PLATFORM
  }
  else
    RP.EnterHeadlessMode();
#endif

  fellowModulesShutdown();
  fellowDeleteServices();
  return EXIT_SUCCESS;
}
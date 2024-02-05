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

#include <time.h>

#include "Defs.h"
#include "versioninfo.h"
#include "FellowMain.h"
#include "chipset.h"
#include "Renderer.h"
#include "CpuModule.h"
#include "CpuIntegration.h"
#include "MemoryInterface.h"
#include "FloppyDisk.h"
#include "Gameports.h"
#include "Keyboard.h"
#include "GraphicsPipeline.h"
#include "Module/Hardfile/IHardfileHandler.h"
#include "Cia.h"
#include "Blitter.h"
#include "Timers.h"
#include "Configuration.h"
#include "WindowsUI.h"
#include "FilesystemIntegration.h"
#include "IniFile.h"
#include "SystemInformation.h"
#include "PaulaInterrupt.h"
#include "RetroPlatform.h"

#include "graphics/Graphics.h"
#include "../automation/Automator.h"

#include "VirtualHost/Core.h"
#include "VirtualHost/CoreFactory.h"

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

/*============================================================================*/
/* The run-time log                                                           */
/*============================================================================*/

constexpr size_t WRITE_LOG_BUF_SIZE = 512;

void fellowShowRequester(FELLOW_REQUESTER_TYPE type, const char *format, ...)
{
  char buffer[WRITE_LOG_BUF_SIZE];
  va_list parms;

  va_start(parms, format);
  _vsnprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
  va_end(parms);

  _core.Log->AddLog(buffer);

#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
#endif
    wguiShowRequester(buffer, type);
}

char *fellowGetVersionString()
{
  char *result = (char *)malloc(strlen(FELLOWVERSION) + 12);

  if (!result)
  {
    return nullptr;
  }

#ifdef X64
  sprintf(result, "%s - %d bit", FELLOWVERSION, 64);
#else
  sprintf(result, "%s - %d bit", FELLOWVERSION, 32);
#endif

  return result;
}

/*============================================================================*/
/* Runtime Error Check                                                        */
/*============================================================================*/

static void fellowRuntimeErrorCheck()
{
  switch (fellowGetRuntimeErrorCode())
  {
    case fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK:
      fellowShowRequester(
          FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR,
          "A serious emulation runtime error occured:\nThe emulated CPU entered Amiga memory that can not hold\nexecutable data. Emulation could not continue.");
      break;
  }
  fellowSetRuntimeErrorCode(fellow_runtime_error_codes::FELLOW_RUNTIME_ERROR_NO_ERROR);
}

/*============================================================================*/
/* Softreset                                                                  */
/*============================================================================*/

void fellowSoftReset()
{
  _core.Scheduler->SoftReset();

  memorySoftReset();
  interruptSoftReset();
  _core.HardfileHandler->HardReset();
  _core.CurrentSprites->HardReset();
  drawHardReset();
  kbdHardReset();
  gameportHardReset();
  // busSoftReset();
  _core.Sound->HardReset();
  blitterHardReset();
  _core.CurrentCopper->HardReset();
  floppyHardReset();
  ciaHardReset();
  graphHardReset();
  ffilesysHardReset();
  memoryHardResetPost();
  fellowSetPreStartReset(false);

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT) GraphicsContext.SoftReset();
}

/*============================================================================*/
/* Hardreset                                                                  */
/*============================================================================*/

void fellowHardReset()
{
  _core.Agnus->HardReset();
  _core.Clocks->HardReset(*_core.CurrentFrameParameters);
  _core.Scheduler->HardReset();

  memoryHardReset();
  interruptHardReset();
  _core.HardfileHandler->HardReset();
  _core.CurrentSprites->HardReset();
  drawHardReset();
  kbdHardReset();
  gameportHardReset();
  // busHardReset();
  _core.Sound->HardReset();
  blitterHardReset();
  _core.CurrentCopper->HardReset();
  floppyHardReset();
  ciaHardReset();
  graphHardReset();
  ffilesysHardReset();
  memoryHardResetPost();
  cpuIntegrationHardReset();
  fellowSetPreStartReset(false);

  // if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT) GraphicsContext.HardReset();
}

/*============================================================================*/
/* Modules use this to request that emulation stop safely.                    */
/* Emulation will stop at the beginning of the next frame                     */
/*============================================================================*/

void fellowRequestEmulationStop()
{
  _core.Scheduler->RequestEmulationStop();
}

void fellowRequestEmulationStopClear()
{
  _core.Scheduler->ClearRequestEmulationStop();
}

/*============================================================================*/
/* Controls the process of starting actual emulation                          */
/*============================================================================*/

bool fellowEmulationStart()
{
  bool cycleAccuracy = drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT;
  _core.ConfigureAccuracy(cycleAccuracy);

  fellowRequestEmulationStopClear();
  iniEmulationStart();
  memoryEmulationStart();
  interruptEmulationStart();
  ciaEmulationStart();
  cpuIntegrationEmulationStart();

  _core.CurrentSprites->EmulationStart();

  blitterEmulationStart();

  _core.CurrentCopper->EmulationStart();

  if (!drawEmulationStart())
  {
    return false;
  }

  kbdEmulationStart();
  gameportEmulationStart();

  if (!drawEmulationStartPost())
  {
    return false;
  }

  graphEmulationStart();
  _core.Sound->EmulationStart();
  // busEmulationStart();
  floppyEmulationStart();
  ffilesysEmulationStart();
  timerEmulationStart();

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.EmulationStart();
#endif

  if (cycleAccuracy) GraphicsContext.EmulationStart();

  _core.Uart->EmulationStart();
  _core.HardfileHandler->EmulationStart();

  return memoryGetKickImageOK();
}

/*============================================================================*/
/* Controls the process of halting actual emulation                           */
/*============================================================================*/

void fellowEmulationStop()
{
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.EmulationStop();
#endif
  _core.HardfileHandler->EmulationStop();
  timerEmulationStop();
  ffilesysEmulationStop();
  floppyEmulationStop();
  // busEmulationStop();
  _core.Sound->EmulationStop();
  gameportEmulationStop();
  kbdEmulationStop();
  drawEmulationStop();
  _core.CurrentCopper->EmulationStop();
  blitterEmulationStop();
  _core.CurrentSprites->EmulationStop();
  graphEmulationStop();
  cpuIntegrationEmulationStop();
  ciaEmulationStop();
  interruptEmulationStop();
  memoryEmulationStop();
  iniEmulationStop();
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT) GraphicsContext.EmulationStop();

  _core.Uart->EmulationStop();
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

  _core.Scheduler->Run();

  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

void fellowStepOne()
{
  fellowRequestEmulationStopClear();
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  _core.Scheduler->DebugStepOneInstruction();

  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

void fellowStepOver()
{
  char saddress[128];
  char sdata[128];
  char sinstruction[128];
  char soperands[128];

  fellowRequestEmulationStopClear();
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  uint32_t current_pc = cpuGetPC();
  uint32_t overPc = cpuDisOpcode(current_pc, saddress, sdata, sinstruction, soperands);

  _core.Scheduler->DebugStepUntilPc(overPc);

  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}

void fellowRunDebug(uint32_t breakpoint)
{
  fellowRequestEmulationStopClear();
  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  _core.Scheduler->DebugStepUntilPc(breakpoint);

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
  _core.Sound->Shutdown();
  fprintf(stderr, "Serious error! Exit.\n");
  exit(EXIT_FAILURE);
}

/*============================================================================*/
/* Draw subsystem failure message and exit                                    */
/*============================================================================*/

static void fellowDrawFailed()
{
  fellowShowRequester(
      FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, "Graphics subsystem failed to start.\nPlease check your OS graphics driver setup.\nClosing down application.");

  exit(EXIT_FAILURE);
}

/*============================================================================*/
/* Inititalize all modules in the emulator, called on startup                 */
/*============================================================================*/

static void fellowModulesStartup(int argc, const char **argv)
{
  CoreFactory::CreateServices();

  sysinfoLogSysInfo();

  CoreFactory::CreateDrivers();
  CoreFactory::CreateDebugVM();
  CoreFactory::CreateModules();

  chipsetStartup();
  timerStartup();
  _core.HardfileHandler->Startup();
  ffilesysStartup();
  iniStartup();
  kbdStartup();
  cfgStartup(argc, argv);
  if (!drawStartup()) fellowDrawFailed();
  gameportStartup();
  // busStartup();
  _core.Sound->Startup();
  blitterStartup();
  floppyStartup();
  ciaStartup();
  memoryStartup();
  interruptStartup();
  graphStartup();
  cpuIntegrationStartup();
  wguiStartup();

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.Startup();
#endif

  // TODO: Deal with this
  // if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT) GraphicsContext.Startup();

  automator.Startup();
}

/*============================================================================*/
/* Release all modules in the emulator, called on shutdown                    */
/*============================================================================*/

static void fellowModulesShutdown()
{
  automator.Shutdown();
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT) GraphicsContext.Shutdown();
#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode()) RP.Shutdown();
#endif
  wguiShutdown();
  cpuIntegrationShutdown();
  graphShutdown();
  interruptShutdown();
  memoryShutdown();
  ciaShutdown();
  floppyShutdown();

  blitterShutdown();
  _core.Sound->Shutdown();
  // busShutdown();
  gameportShutdown();
  drawShutdown();
  cfgShutdown();
  kbdShutdown();
  iniShutdown();
  ffilesysShutdown();
  _core.HardfileHandler->Shutdown();
  timerShutdown();

  CoreFactory::DestroyModules();
  CoreFactory::DestroyDebugVM();
  CoreFactory::DestroyDrivers();
  CoreFactory::DestroyServices();
}

/*============================================================================*/
/* main....                                                                   */
/*============================================================================*/

int __cdecl main(int argc, const char *argv[])
{
  fellowSetPreStartReset(true);
  fellowModulesStartup(argc, argv);

#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
#endif
    // set DPI awareness in standalone GUI mode to system DPI aware
    wguiSetProcessDPIAwareness("2");
    while (!wguiEnter())
      fellowRun();
#ifdef RETRO_PLATFORM
  }
  else
    RP.EnterHeadlessMode();
#endif

  fellowModulesShutdown();

  return EXIT_SUCCESS;
}
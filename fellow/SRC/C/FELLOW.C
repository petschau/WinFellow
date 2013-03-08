/* @(#) $Id: FELLOW.C,v 1.39 2013-01-13 18:31:09 peschau Exp $ */
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
#include <Windows.h>
#include <Dbghelp.h>

#include "defs.h"
#include "versioninfo.h"
#include "fellow.h"
#include "draw.h"
#include "CpuModule.h"
#include "CpuModule_Disassembler.h"
#include "CpuIntegration.h"
#include "fmem.h"
#include "eventid.h"
#include "floppy.h"
#include "sound.h"
#include "gameport.h"
#include "kbd.h"
#include "graph.h"
#include "fhfile.h"
#include "bus.h"
#include "copper.h"
#include "cia.h"
#include "blit.h"
#include "sprite.h"
#include "fswrap.h"
#include "timer.h"
#include "config.h"
#include "wgui.h"
#include "ffilesys.h"
#include "ini.h"
#include "sysinfo.h"
#include "fileops.h"
#include "RetroPlatform.h"

BOOLE fellow_request_emulation_stop;

char fellowlogfilename[MAX_PATH];

/*============================================================================*/
/* Perform reset before starting emulation flag                               */
/*============================================================================*/

BOOLE fellow_pre_start_reset;

void fellowPreStartReset(BOOLE reset) {
  fellow_pre_start_reset = reset;
}

BOOLE fellowGetPreStartReset(void) {
  return fellow_pre_start_reset;
}


/*============================================================================*/
/* Using GUI                                                                  */
/*============================================================================*/

static BOOLE fellow_use_gui;

void fellowSetUseGUI(BOOLE use_gui) {
  fellow_use_gui = use_gui;
}

BOOLE fellowGetUseGUI(void) {
  return fellow_use_gui;
}


/*============================================================================*/
/* We have MMX?                                                               */
/*============================================================================*/

BOOLE fellow_mmx_detected;

void fellowSetMMXDetected(BOOLE mmx_detected) {
  fellow_mmx_detected = mmx_detected;
}

BOOLE fellowGetMMXDetected(void) {
  return fellow_mmx_detected;
}


/*============================================================================*/
/* setjmp support                                                             */
/*============================================================================*/

static jmp_buf fellow_runtime_error_env;
static fellow_runtime_error_codes fellow_runtime_error_code;

void fellowSetRuntimeErrorCode(fellow_runtime_error_codes error_code) {
  fellow_runtime_error_code = error_code;
}


static fellow_runtime_error_codes fellowGetRuntimeErrorCode(void) {
  return fellow_runtime_error_code;
}


/*============================================================================*/
/* The run-time log                                                           */
/*============================================================================*/

#define WRITE_LOG_BUF_SIZE 512

static BOOLE fellow_log_first_time = TRUE;
static BOOLE fellow_log_enabled;
static BOOLE fellow_newlogline = TRUE;

static void fellowSetLogEnabled(BOOLE enabled) {
  fellow_log_enabled = enabled;
}

static BOOLE fellowGetLogEnabled(void) {
  return fellow_log_enabled;
}

static void fellowSetLogFirstTime(BOOLE first_time) {
  fellow_log_first_time = first_time;
}

static BOOLE fellowGetLogFirstTime(void) {
  return fellow_log_first_time;
}

void fellowAddLog2(STR *msg) {
  FILE *F;

  if (!fellowGetLogEnabled()) 
    return;

  if (fellowGetLogFirstTime()) {
    fileopsGetFellowLogfileName(fellowlogfilename);
    F = fopen(fellowlogfilename, "w");
    fellowSetLogFirstTime(FALSE);
  }
  else {
    F = fopen(fellowlogfilename, "a");
  }

  if (F != NULL) {
    fprintf(F, "%s", msg);
    fflush(F);
    fclose(F);
  }
}

static void fellowLogDateTime(void)
{
  char tmp[255];
  time_t thetime;
  struct tm *timedata;

  thetime = time(NULL);
  timedata = localtime(&thetime);

  strftime(tmp, 255, "%c: ", timedata);
  fellowAddLog2(tmp);
}

void fellowAddLog(const char *format,...)
{
  char buffer[WRITE_LOG_BUF_SIZE];
  va_list parms;
  int count = 0;

  va_start (parms, format);
  count = _vsnprintf( buffer, WRITE_LOG_BUF_SIZE-1, format, parms );

  if(fellow_newlogline)
    fellowLogDateTime();

  fellowAddLog2(buffer);

  if(buffer[strlen(buffer)-1] == '\n')
    fellow_newlogline = TRUE;
  else
    fellow_newlogline = FALSE;
  va_end (parms);
}

void fellowAddTimelessLog(const char *format,...)
{
  char buffer[WRITE_LOG_BUF_SIZE];
  va_list parms;
  int count = 0;

  va_start (parms, format);
  count = _vsnprintf( buffer, WRITE_LOG_BUF_SIZE-1, format, parms );

  fellowAddLog2(buffer);

  if(buffer[strlen(buffer)-1] == '\n')
    fellow_newlogline = TRUE;
  else
    fellow_newlogline = FALSE;

  va_end (parms);
}

char *fellowGetVersionString(void)
{
  char *result = (char *) malloc(strlen(FELLOWVERSION)+ strlen(__DATE__) + 4);

  if(!result)
    return NULL;

  //    sprintf(result, "%s (%s)", FELLOWVERSION, __DATE__);
  sprintf(result, "%s", FELLOWVERSION);
  return result;
}


/*============================================================================*/
/* Runtime Error Check                                                        */
/*============================================================================*/

static void fellowRuntimeErrorCheck(void) {
  switch (fellowGetRuntimeErrorCode()) {
    case FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK:
      wguiRequester("A serious emulation runtime error occured:",
	"The emulated CPU entered Amiga memory that can not hold",
	"executable data. Emulation could not continue.");
      break;
  }
  fellowSetRuntimeErrorCode(FELLOW_RUNTIME_ERROR_NO_ERROR);
}


/*============================================================================*/
/* Softreset                                                                  */
/*============================================================================*/

void fellowSoftReset(void) {
  memorySoftReset();
  fhfileHardReset();
  spriteHardReset();
  drawHardReset();
  kbdHardReset();
  gameportHardReset();
  busSoftReset();
  soundHardReset();
  blitterHardReset();
  copperHardReset();
  floppyHardReset();
  ciaHardReset();
  graphHardReset();
  ffilesysHardReset();
  memoryHardResetPost();
  fellowPreStartReset(FALSE);
}

/*============================================================================*/
/* Hardreset                                                                  */
/*============================================================================*/

void fellowHardReset(void) {
  memoryHardReset();
  fhfileHardReset();
  spriteHardReset();
  drawHardReset();
  kbdHardReset();
  gameportHardReset();
  busHardReset();
  soundHardReset();
  blitterHardReset();
  copperHardReset();
  floppyHardReset();
  ciaHardReset();
  graphHardReset();
  ffilesysHardReset();
  memoryHardResetPost();
  cpuIntegrationHardReset();
  fellowPreStartReset(FALSE);
}

/*============================================================================*/
/* Modules use this to request that emulation stop safely.                    */
/* Emulation will stop at the beginning of the next frame                     */
/*============================================================================*/

void fellowRequestEmulationStop(void) {
  fellow_request_emulation_stop = TRUE;
}


void fellowRequestEmulationStopClear(void) {
  fellow_request_emulation_stop = FALSE;
}


/*============================================================================*/
/* Controls the process of starting actual emulation                          */
/*============================================================================*/

BOOLE fellowEmulationStart(void) {
  BOOLE result;
  fellowRequestEmulationStopClear();
  iniEmulationStart();
  memoryEmulationStart();
  ciaEmulationStart();
  cpuIntegrationEmulationStart();
  graphEmulationStart();
  spriteEmulationStart();
  blitterEmulationStart();
  copperEmulationStart();
  drawEmulationStart();
  kbdEmulationStart();
  gameportEmulationStart();
  result = drawEmulationStartPost();
  soundEmulationStart();
  busEmulationStart();
  floppyEmulationStart();
  ffilesysEmulationStart();
  timerEmulationStart();
#ifdef RETRO_PLATFORM
  if(RetroPlatformGetMode())
    RetroPlatformEmulationStart();
#endif
  return result && memoryGetKickImageOK();
}


/*============================================================================*/
/* Controls the process of halting actual emulation                           */
/*============================================================================*/

void fellowEmulationStop(void) {
#ifdef RETRO_PLATFORM
  if(RetroPlatformGetMode())
    RetroPlatformEmulationStop();
#endif
  timerEmulationStop();
  ffilesysEmulationStop();
  floppyEmulationStop();
  busEmulationStop();
  soundEmulationStop();
  gameportEmulationStop();
  kbdEmulationStop();
  drawEmulationStop();
  copperEmulationStop();
  blitterEmulationStop();
  spriteEmulationStop();
  graphEmulationStop();
  cpuIntegrationEmulationStop();
  ciaEmulationStop();
  memoryEmulationStop();
  iniEmulationStop();
}

/*============================================================================*/
/* Starts emulation                                                           */
/* FellowEmulationStart() has already been called                             */
/* FellowEmulationStop() will be called elsewhere                             */
/*============================================================================*/

void fellowRun(void) {
  if (fellow_pre_start_reset) fellowHardReset();
  fellowSetRuntimeErrorCode((fellow_runtime_error_codes) setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == FELLOW_RUNTIME_ERROR_NO_ERROR)
    busRun();
  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}


/*============================================================================*/
/* Steps one CPU instruction                                                  */
/*============================================================================*/

void fellowStepOne(void)
{
  fellowRequestEmulationStopClear();
  if (fellow_pre_start_reset)
  {
    fellowHardReset();
  }
  fellowSetRuntimeErrorCode((fellow_runtime_error_codes) setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    busDebugStepOneInstruction();
  }
  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}


/*============================================================================*/
/* Steps over a CPU instruction                                               */
/*============================================================================*/

void fellowStepOver(void)
{
  char saddress[128];
  char sdata[128];
  char sinstruction[128];
  char soperands[128];

  fellowRequestEmulationStopClear();
  if (fellow_pre_start_reset)
  {
    fellowHardReset();
  }
  
  fellowSetRuntimeErrorCode((fellow_runtime_error_codes)setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == FELLOW_RUNTIME_ERROR_NO_ERROR)
  {
    ULO current_pc = cpuGetPC();
    ULO over_pc = cpuDisOpcode(current_pc, saddress, sdata, sinstruction, soperands);
    while ((cpuGetPC() != over_pc) && !fellow_request_emulation_stop)
    {
      busDebugStepOneInstruction();
    }
  }
  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}


/*============================================================================*/
/* Run until we crash or is exited in debug-mode                              */
/*============================================================================*/

void fellowRunDebug(ULO breakpoint) {
  fellowRequestEmulationStopClear();
  if (fellow_pre_start_reset) fellowHardReset();
  fellowSetRuntimeErrorCode((fellow_runtime_error_codes) setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == FELLOW_RUNTIME_ERROR_NO_ERROR)
    while ((!fellow_request_emulation_stop) && (breakpoint != cpuGetPC()))
      busDebugStepOneInstruction();
  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}


/*============================================================================*/
/* Something really bad happened, immediate emulation stop                    */
/*============================================================================*/

void fellowNastyExit(void) {
  longjmp(fellow_runtime_error_env, fellowGetRuntimeErrorCode());  
  fprintf(stderr, "You only die twice, I give in!\n");
  soundShutdown();
  fprintf(stderr, "Serious error! Exit.\n");
  fsNavigSetCWDStartupDir();
  exit(EXIT_FAILURE);
}


/*============================================================================*/
/* Draw subsystem failure message and exit                                    */
/*============================================================================*/

static void fellowDrawFailed(void) {
  wguiRequester("Graphics subsystem failed to start. ",
    "Please check your OS graphics driver setup. ",
    "Closing down application.");
  exit(EXIT_FAILURE);
}

/*============================================================================*/
/* Save statefile                                                             */
/*============================================================================*/

BOOLE fellowSaveState(STR *filename)
{
  FILE *F = fopen(filename, "wb");
  
  if (F == NULL) return FALSE;

  cpuIntegrationSaveState(F);
  memorySaveState(F);
  copperSaveState(F);
  busSaveState(F);
  blitterSaveState(F);
  ciaSaveState(F);

  fclose(F);
  return TRUE;
}

/*============================================================================*/
/* Load statefile                                                             */
/*============================================================================*/

BOOLE fellowLoadState(STR *filename)
{
  FILE *F = fopen(filename, "rb");
  
  if (F == NULL) return FALSE;

  cpuIntegrationLoadState(F);
  memoryLoadState(F);
  copperLoadState(F);
  busLoadState(F);
  blitterLoadState(F);
  ciaLoadState(F);

  fclose(F);
  return TRUE;
}

/*============================================================================*/
/* Inititalize all modules in the emulator, called on startup                 */
/*============================================================================*/

static void fellowModulesStartup(int argc, char *argv[])
{
  timerStartup();
  fsNavigStartup(argv);
  fhfileStartup();
  ffilesysStartup();
  spriteStartup();
  iniStartup();
  kbdStartup();
  cfgStartup(argc, argv);
  if (!drawStartup()) 
    fellowDrawFailed();
  gameportStartup();
  busStartup();
  soundStartup();
  blitterStartup();
  copperStartup();
  floppyStartup();
  ciaStartup();
  memoryStartup();
  graphStartup();
  cpuIntegrationStartup();
  wguiStartup();
#ifdef RETRO_PLATFORM
  if(RetroPlatformGetMode())
    RetroPlatformStartup();
#endif
}

/*============================================================================*/
/* Release all modules in the emulator, called on shutdown                    */
/*============================================================================*/

static void fellowModulesShutdown(void)
{
#ifdef RETRO_PLATFORM
  if(RetroPlatformGetMode())
	  RetroPlatformShutdown();
#endif
  wguiShutdown();
  cpuIntegrationShutdown();
  graphShutdown();
  memoryShutdown();
  ciaShutdown();
  floppyShutdown();
  copperShutdown();
  blitterShutdown();
  soundShutdown();
  busShutdown();
  gameportShutdown();
  drawShutdown();
  cfgShutdown();
  kbdShutdown();
  iniShutdown();
  spriteShutdown();
  ffilesysShutdown();
  fhfileShutdown();
  fsNavigSetCWDStartupDir();
  fsNavigShutdown();
  timerShutdown();
}

void fellowWriteMinidump(EXCEPTION_POINTERS* e) {
  char name[MAX_PATH], filename[MAX_PATH];
  auto hDbgHelp = LoadLibraryA("dbghelp");
  SYSTEMTIME t;

  if(hDbgHelp == nullptr)
      return;

  auto pMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

  if(pMiniDumpWriteDump == nullptr)
      return;

  GetSystemTime(&t);

  wsprintfA(filename, "WinFellow_%s_%4d%02d%02d_%02d%02d%02d.dmp", 
    FELLOWNUMERICVERSION, 
    t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

  fileopsGetGenericFileName(name, "WinFellow", filename);

  fellowAddLog("Unhandled exception detected, write minidump to %s...\n", name);

  auto hFile = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  if(hFile == INVALID_HANDLE_VALUE)
      return;

  MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
  exceptionInfo.ThreadId = GetCurrentThreadId();
  exceptionInfo.ExceptionPointers = e;
  exceptionInfo.ClientPointers = FALSE;

  auto dumped = pMiniDumpWriteDump(
    GetCurrentProcess(),
    GetCurrentProcessId(),
    hFile,
    MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
    e ? &exceptionInfo : nullptr,
    nullptr,
    nullptr);

  CloseHandle(hFile);

  return;
}

LONG CALLBACK fellowUnhandledExceptionHandler(EXCEPTION_POINTERS* e) {
  fellowWriteMinidump(e);
  return EXCEPTION_CONTINUE_SEARCH;
}

/*============================================================================*/
/* main....                                                                   */
/*============================================================================*/

int __cdecl main(int argc, char *argv[]) {
 SetUnhandledExceptionFilter(fellowUnhandledExceptionHandler);

#ifdef _FELLOW_DEBUG_CRT_MALLOC
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  fellowSetLogFirstTime(TRUE);
  fellowSetLogEnabled(TRUE);

  sysinfoLogSysInfo();
  fellowPreStartReset(TRUE);
  fellowSetMMXDetected(sysinfoDetectMMX());
  fellowModulesStartup(argc, argv);

#ifdef RETRO_PLATFORM
  if(!RetroPlatformGetMode())
#endif
    while (!wguiEnter())
      fellowRun();
#ifdef RETRO_PLATFORM
  else
    RetroPlatformEnter();
#endif
  
  fellowModulesShutdown();
  return EXIT_SUCCESS;
}
/* @(#) $Id: FELLOW.C,v 1.15.2.11.4.1 2008-02-03 12:37:29 peschau Exp $ */
/*=========================================================================*/
/* Fellow Amiga Emulator                                                   */
/*                                                                         */
/* This file has overall control over starting and stopping Fellow         */
/* Everything starts here                                                  */
/*                                                                         */
/* Author: Petter Schau (peschau@online.no)                                */
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

#include "defs.h"
#include "fellow.h"
#include "draw.h"
#include "cpu.h"
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
#include "cpudis.h"
#include "sysinfo.h"

BOOLE fellow_request_emulation_stop;
BOOLE fellow_request_emulation_stop_immediately;


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

#define WRITE_LOG_BUF_SIZE 128

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

  if (!fellowGetLogEnabled()) return;
  if (fellowGetLogFirstTime()) {
    F = fopen("fellow.log", "w");
    fellowSetLogFirstTime(FALSE);
  }
  else F = fopen("fellow.log", "a");
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
  cpuHardReset();
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
/* Modules can use this to request that emulation stop immediately            */
/* Only the debugger should use this.                                         */
/*============================================================================*/

void fellowRequestEmulationStopImmediately(void) {
  fellow_request_emulation_stop_immediately = TRUE;
}

void fellowRequestEmulationStopImmediatelyClear(void) {
  fellow_request_emulation_stop_immediately = FALSE;
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
  cpuEmulationStart();
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
  return result && memoryGetKickImageOK();
}


/*============================================================================*/
/* Controls the process of halting actual emulation                           */
/*============================================================================*/

void fellowEmulationStop(void) {
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
  cpuEmulationStop();
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
  fellowRequestEmulationStopImmediatelyClear();
  if (fellow_pre_start_reset) fellowHardReset();
  fellowSetRuntimeErrorCode(setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == FELLOW_RUNTIME_ERROR_NO_ERROR)
    busRunNew();
  fellowRequestEmulationStopImmediatelyClear();
  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}


/*============================================================================*/
/* Steps one CPU instruction                                                  */
/*============================================================================*/

void fellowStepOne(void) {
  fellowRequestEmulationStopImmediatelyClear();
  fellowRequestEmulationStopClear();
  if (fellow_pre_start_reset) fellowHardReset();
  fellowSetRuntimeErrorCode(setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == FELLOW_RUNTIME_ERROR_NO_ERROR)
    busDebugNew();
  fellowRequestEmulationStopImmediatelyClear();
  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}


/*============================================================================*/
/* Steps over a CPU instruction                                               */
/*============================================================================*/

void fellowStepOver(void) {
  char saddress[128];
  char sdata[128];
  char sinstruction[128];
  char soperands[128];
  fellowRequestEmulationStopImmediatelyClear();
  fellowRequestEmulationStopClear();
  if (fellow_pre_start_reset) fellowHardReset();
  fellowSetRuntimeErrorCode(setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == FELLOW_RUNTIME_ERROR_NO_ERROR) {
    ULO current_pc = cpuGetPC();
    ULO over_pc = cpuDisOpcode(current_pc, saddress, sdata, sinstruction, soperands);
    while ((cpuGetPC() != over_pc) && (cpuGetPC() != 0xf80360) && !fellow_request_emulation_stop)
      busDebugNew();
  }
  fellowRequestEmulationStopImmediatelyClear();
  fellowRequestEmulationStopClear();
  fellowRuntimeErrorCheck();
}


/*============================================================================*/
/* Run until we crash or is exited in debug-mode                              */
/*============================================================================*/

void fellowRunDebug(ULO breakpoint) {
  fellowRequestEmulationStopImmediatelyClear();
  fellowRequestEmulationStopClear();
  if (fellow_pre_start_reset) fellowHardReset();
  fellowSetRuntimeErrorCode(setjmp(fellow_runtime_error_env));
  if (fellowGetRuntimeErrorCode() == FELLOW_RUNTIME_ERROR_NO_ERROR)
    while ((!fellow_request_emulation_stop) && (breakpoint != cpuGetPC()))
      busDebugNew();
  fellowRequestEmulationStopImmediatelyClear();
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
/* Inititalize all modules in the emulator, called on startup                 */
/*============================================================================*/

static void fellowModulesStartup(int argc, char *argv[]) {
  timerStartup();
  fsNavigStartup(argv);
  fhfileStartup();
  ffilesysStartup();
  spriteStartup();
  iniStartup();
  if (!drawStartup()) fellowDrawFailed();
  kbdStartup();
  gameportStartup();
  busStartup();
  soundStartup();
  blitterStartup();
  copperStartup();
  floppyStartup();
  ciaStartup();
  memoryStartup();
  graphStartup();
  cpuStartup();
  cfgStartup(argc, argv);
  wguiStartup();
}


/*============================================================================*/
/* Release all modules in the emulator, called on shutdown                    */
/*============================================================================*/

static void fellowModulesShutdown(void) {
  wguiShutdown();
  cfgShutdown();
  cpuShutdown();
  graphShutdown();
  memoryShutdown();
  ciaShutdown();
  floppyShutdown();
  copperShutdown();
  blitterShutdown();
  soundShutdown();
  busShutdown();
  gameportShutdown();
  kbdShutdown();
  drawShutdown();
  iniShutdown();
  spriteShutdown();
  ffilesysShutdown();
  fhfileShutdown();
  fsNavigSetCWDStartupDir();
  fsNavigShutdown();
  timerShutdown();
}


/*============================================================================*/
/* main....                                                                   */
/*============================================================================*/

int __cdecl main(int argc, char *argv[]) {

  #ifdef _FELLOW_DEBUG_CRT_MALLOC
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  #endif

  fellowSetLogFirstTime(TRUE);
  fellowSetLogEnabled(TRUE);

  sysinfoLogSysInfo();
  fellowPreStartReset(TRUE);
  fellowSetMMXDetected(sysinfoDetectMMX());
  fellowModulesStartup(argc, argv);
  while (!wguiEnter())
    fellowRun();
  fellowModulesShutdown();
  return EXIT_SUCCESS;
}
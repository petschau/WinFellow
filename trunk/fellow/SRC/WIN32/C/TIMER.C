/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Provides timer routines                                                   */
/* Author:  Petter Schau (peschau@online.no)                                 */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

#include "defs.h"

/*===========================================================================*/
/* Returns current time in ms                                                */
/*===========================================================================*/

ULO timerGetTimeMs(void) {
  return timeGetTime();
}

/*===========================================================================*/
/* Fellow module functions                                                   */
/*===========================================================================*/

void timerEmulationStart(void) {
}

void timerEmulationStop(void) {
}

void timerStartup(void) {
}

void timerShutdown(void) {
}

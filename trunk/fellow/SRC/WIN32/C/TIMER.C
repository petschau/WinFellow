/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Provides timer routines                                                   */
/* Author:  Petter Schau (peschau@online.no)                                 */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

#include "defs.h"

/* This module is in a experimental state, have not decided for final layout */

ULO tick, tick2, tock;


BOOLE timer_has_perf_counter;
ULO timer_perf_counter_freq;
LLO timer_perf_counter_start;


int testcounter;
LLO testtime1;
LLO testtime2;
LLO testtimetotal;


void testtimestart(void) {
  QueryPerformanceCounter((union _LARGE_INTEGER *) &testtime1);
}

void testtimestop(void) {
  QueryPerformanceCounter((union _LARGE_INTEGER *) &testtime2);
  testtimetotal += testtime2 - testtime1;
  testcounter++;
}


/*===========================================================================*/
/* Returns current time in ms                                                */
/*===========================================================================*/

ULO timerTimeGet(void) {
  if (timer_has_perf_counter) {
    LLO mytime;

    QueryPerformanceCounter((union _LARGE_INTEGER *) &mytime);
    return ((mytime - timer_perf_counter_start)*1000)/timer_perf_counter_freq;
  }
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

  timer_has_perf_counter = QueryPerformanceFrequency((union _LARGE_INTEGER *) &timer_perf_counter_freq);  
  if (timer_has_perf_counter)
    QueryPerformanceCounter((union _LARGE_INTEGER *) &timer_perf_counter_start);

  testcounter = 0;
  testtimetotal = 0;
}

void timerShutdown(void) {
}


/*===========================================================================*/
/* Old code                                                                  */
/*===========================================================================*/


#ifdef WITH_OS_TIMER

/*=======================================*/
/* Generic frames per second calculation */
/*=======================================*/

ULO fps_lastcc, fps_nowcc;
ULO fps;

void timerFPSCalculate(void) {
/*
  fps_nowcc = (frames*71364) + (ypos*228) + xpos;
  fps = ((fps_nowcc - fps_lastcc)*50)/(71364*sound_buffer_depth);
  fps_lastcc = fps_nowcc;
*/
}
	 


/* Using _dos_setvect etc. to set timer irq.  Assume dos always has the */
/* timer running so don't enable in the mask */

/* Two routines: */
/* timer_routine_continuous - will provide real-time sound-emulation with */
/*                            real-time synchronization + update speedmtr */
/* timer_routine - use when sound is not played for 50hz synchronization */


void start_timerirq(void) {
}

void stop_timerirq(void) {
}

#endif


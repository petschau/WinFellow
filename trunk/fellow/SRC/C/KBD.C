/*============================================================================*/
/* Fellow Emulator                                                            */
/* Keyboard emulation                                                         */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/


//#include <dos.h>
#include "defs.h"
#include "fellow.h"
#include "keycodes.h"
#include "kbd.h"
#include "kbddrv.h"
#include "gameport.h"
#include "graph.h"
#include "cia.h"
#include "draw.h"


/*===========================================================================*/
/* Data collected in struct, for easy locking in IRQs by kbddrv.c            */
/*===========================================================================*/

kbd_state_type kbd_state;
ULO kbd_time_to_wait;


/*===========================================================================*/
/* Stuff that later goes other places                                        */
/*===========================================================================*/

ULO volatile f12pressed;
UBY insert_dfX[4];                         /* 0 - nothing 1- insert 2-eject */


/*===========================================================================*/
/* Called from the main end of frame handler                                 */
/*===========================================================================*/

void kbdEventEOFHandler(void) {
  kbd_event thisev;
  
  while (kbd_state.eventsEOF.outpos < kbd_state.eventsEOF.inpos) {
    thisev =(kbd_event)(kbd_state.eventsEOF.buffer[kbd_state.eventsEOF.outpos &
						  KBDBUFFERMASK]);
    switch (thisev) {
      case EVENT_INSERT_DF0:
	insert_dfX[0] = 1;
	f12pressed = TRUE;
	break;
      case EVENT_INSERT_DF1:
	insert_dfX[1] = 1;
	f12pressed = TRUE;
	break;
      case EVENT_INSERT_DF2:
	insert_dfX[2] = 1;
	f12pressed = TRUE;
	break;
      case EVENT_INSERT_DF3:
	insert_dfX[3] = 1;
	f12pressed = TRUE;
	break;
      case EVENT_EJECT_DF0:
	insert_dfX[0] = 2;
	f12pressed = TRUE;
	break;
      case EVENT_EJECT_DF1:
	insert_dfX[1] = 2;
	f12pressed = TRUE;
	break;
      case EVENT_EJECT_DF2:
	insert_dfX[2] = 2;
	f12pressed = TRUE;
	break;
      case EVENT_EJECT_DF3:
	insert_dfX[3] = 2;
	f12pressed = TRUE;
	break;
      case EVENT_EXIT:
	f12pressed = TRUE;
	break;
      case EVENT_SCROLL_UP:
	draw_view_scroll = 0x48;
	break;
      case EVENT_SCROLL_DOWN:
	draw_view_scroll = 0x50;
	break;
      case EVENT_SCROLL_LEFT:
	draw_view_scroll = 0x4b;
	break;
      case EVENT_SCROLL_RIGHT:
	draw_view_scroll = 0x4d;
	break;
      case EVENT_HARD_RESET:
	break;
    }
    kbd_state.eventsEOF.outpos++;
  }
}


/*===========================================================================*/
/* Called from the main end of line handler                                  */
/*===========================================================================*/

void kbdEventEOLHandler(void) {
  kbd_event thisev;
  BOOLE left[2], up[2], right[2], down[2], fire0[2], fire1[2];
  BOOLE left_changed[2], up_changed[2], right_changed[2], down_changed[2];
  BOOLE fire0_changed[2], fire1_changed[2];
  ULO i;
  
  for (i = 0; i < 2; i++)
    left_changed[i] = up_changed[i] = right_changed[i] = down_changed[i] =
    fire0_changed[i] = fire1_changed[i] = FALSE;

  while (kbd_state.eventsEOL.outpos < kbd_state.eventsEOL.inpos) {
    thisev =(kbd_event)(kbd_state.eventsEOL.buffer[kbd_state.eventsEOL.outpos &
						  KBDBUFFERMASK]);
    switch (thisev) {
      case EVENT_JOY0_UP_ACTIVE:
      case EVENT_JOY0_UP_INACTIVE:
	up[0] = (thisev == EVENT_JOY0_UP_ACTIVE);
	up_changed[0] = TRUE;
	fellowAddLog(up[0] ? "Up0 - pressed\n" : "Up0 - released\n");
	break;
      case EVENT_JOY0_DOWN_ACTIVE:
      case EVENT_JOY0_DOWN_INACTIVE:
	down[0] = (thisev == EVENT_JOY0_DOWN_ACTIVE);
	down_changed[0] = TRUE;
	fellowAddLog(down[0] ? "Down0 - pressed\n" : "Down0 - released\n");
	break;
      case EVENT_JOY0_LEFT_ACTIVE:
      case EVENT_JOY0_LEFT_INACTIVE:
	left[0] = (thisev == EVENT_JOY0_LEFT_ACTIVE);
	left_changed[0] = TRUE;
	fellowAddLog(left[0] ? "Left0 - pressed\n" : "Left0 - released\n");
	break;
      case EVENT_JOY0_RIGHT_ACTIVE:
      case EVENT_JOY0_RIGHT_INACTIVE:
	right[0] = (thisev == EVENT_JOY0_RIGHT_ACTIVE);
	right_changed[0] = TRUE;
	fellowAddLog(right[0] ? "Right0 - pressed\n" : "Right0 - released\n");
	break;
      case EVENT_JOY0_FIRE0_ACTIVE:
      case EVENT_JOY0_FIRE0_INACTIVE:
	fire0[0] = (thisev == EVENT_JOY0_FIRE0_ACTIVE);
	fire0_changed[0] = TRUE;
	fellowAddLog(fire0[0] ? "Fire00 - pressed\n" : "Fire00 - released\n");
	break;
      case EVENT_JOY0_FIRE1_ACTIVE:
      case EVENT_JOY0_FIRE1_INACTIVE:
	fire1[0] = (thisev == EVENT_JOY0_FIRE1_ACTIVE);
	fire1_changed[0] = TRUE;
	fellowAddLog(fire1[0] ? "Fire10 - pressed\n" : "Fire10 - released\n");
	break;
      case EVENT_JOY1_UP_ACTIVE:
      case EVENT_JOY1_UP_INACTIVE:
	up[1] = (thisev == EVENT_JOY1_UP_ACTIVE);
	up_changed[1] = TRUE;
	fellowAddLog(up[1] ? "Up1 - pressed\n" : "Up1 - released\n");
	break;
      case EVENT_JOY1_DOWN_ACTIVE:
      case EVENT_JOY1_DOWN_INACTIVE:
	down[1] = (thisev == EVENT_JOY1_DOWN_ACTIVE);
	down_changed[1] = TRUE;
	fellowAddLog(down[1] ? "Down1 - pressed\n" : "Down1 - released\n");
	break;
      case EVENT_JOY1_LEFT_ACTIVE:
      case EVENT_JOY1_LEFT_INACTIVE:
	left[1] = (thisev == EVENT_JOY1_LEFT_ACTIVE);
	left_changed[1] = TRUE;
	fellowAddLog(left[1] ? "Left1 - pressed\n" : "Left1 - released\n");
	break;
      case EVENT_JOY1_RIGHT_ACTIVE:
      case EVENT_JOY1_RIGHT_INACTIVE:
	right[1] = (thisev == EVENT_JOY1_RIGHT_ACTIVE);
	right_changed[1] = TRUE;
	fellowAddLog(right[1] ? "Right1 - pressed\n" : "Right1 - released\n");
	break;
      case EVENT_JOY1_FIRE0_ACTIVE:
      case EVENT_JOY1_FIRE0_INACTIVE:
	fire0[1] = (thisev == EVENT_JOY1_FIRE0_ACTIVE);
	fire0_changed[1] = TRUE;
	fellowAddLog(fire0[1] ? "Fire01 - pressed\n" : "Fire01 - released\n");
	break;
      case EVENT_JOY1_FIRE1_ACTIVE:
      case EVENT_JOY1_FIRE1_INACTIVE:
	fire1[1] = (thisev == EVENT_JOY1_FIRE1_ACTIVE);
	fire1_changed[1] = TRUE;
	fellowAddLog(fire1[1] ? "Fire11 - pressed\n" : "Fire11 - released\n");
	break;
    }
    kbd_state.eventsEOL.outpos++;
  }
  for (i = 0; i < 2; i++)
    if (left_changed[i] ||
	up_changed[i] ||
	right_changed[i] ||
	down_changed[i] ||
	fire0_changed[i] ||
	fire1_changed[i])
      gameportJoystickHandler((i == 0) ? GP_JOYKEY0 : GP_JOYKEY1,
			    (left_changed[i]) ? left[i] : gameport_left[i],
			    (up_changed[i]) ? up[i] : gameport_up[i],
			    (right_changed[i]) ? right[i] : gameport_right[i],
			    (down_changed[i]) ? down[i] : gameport_down[i],
			    (fire0_changed[i]) ? fire0[i] : gameport_fire0[i],
			    (fire1_changed[i]) ? fire1[i] : gameport_fire1[i]);
}


/*===========================================================================*/
/* Move a scancode to the CIA SP register                                    */
/* Called from end_of_line                                                   */
/*===========================================================================*/

void kbdQueueHandler(void) {
  if (kbd_state.scancodes.outpos < kbd_state.scancodes.inpos) {
    if (--kbd_time_to_wait <= 0) {
      ULO scode;

      kbd_time_to_wait = 10;
      scode = kbd_state.scancodes.buffer[kbd_state.scancodes.outpos &
					 KBDBUFFERMASK];
      kbd_state.scancodes.outpos++;
      ciaWritesp(0, ~(((scode >> 7) & 1) | (scode << 1)));
      ciaRaiseIRQC(0, 8);
    }
  }
}


/*===========================================================================*/
/* Fellow standard functions                                                 */
/*===========================================================================*/

void kbdHardReset(void) {
  kbd_state.eventsEOL.inpos = 0;
  kbd_state.eventsEOL.outpos = 0;
  kbd_state.eventsEOF.inpos = 0;
  kbd_state.eventsEOF.outpos = 0;
  kbd_state.scancodes.inpos = 2;
  kbd_state.scancodes.outpos = 0;
  kbd_state.scancodes.buffer[0] = 0xfd; /* Start of keys pressed on reset */
  kbd_state.scancodes.buffer[1] = 0xfe; /* End of keys pressed during reset */
  kbd_time_to_wait = 10;
  f12pressed = FALSE;
  kbdDrvHardReset();
}

void kbdEmulationStart(void) {
  ULO i;

  for (i = 0; i < 4; i++) insert_dfX[i] = FALSE;
  kbdDrvEmulationStart();
}

void kbdEmulationStop(void) {
  kbdDrvEmulationStop();
}

void kbdStartup(void) {
  kbdDrvStartup();
}

void kbdShutdown(void) {
  kbdDrvShutdown();
}





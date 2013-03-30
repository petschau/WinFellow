/* @(#) $Id: SOUND.H,v 1.6 2008-02-21 00:00:44 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Sound emulation                                                         */
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

#ifndef SOUND_H
#define SOUND_H


/*===========================*/
/* Symbols for configuration */
/*===========================*/

typedef enum {SOUND_15650, SOUND_22050, SOUND_31300, SOUND_44100} sound_rates;
typedef enum {SOUND_NONE, SOUND_PLAY, SOUND_EMULATE} sound_emulations;
typedef enum {SOUND_FILTER_ORIGINAL, SOUND_FILTER_ALWAYS, SOUND_FILTER_NEVER} sound_filters;
typedef enum {SOUND_DSOUND_NOTIFICATION, SOUND_MMTIMER_NOTIFICATION} sound_notifications;


/*==========================*/
/* Sound device information */
/*==========================*/

typedef struct {
  BOOLE mono;
  BOOLE stereo;
  BOOLE bits8;
  BOOLE bits16;
  ULO rates_max[2][2]; /* Maximum playback rate for [8/16 bits][mono/stereo] */
} sound_device;


extern sound_emulations sound_emulation;
extern sound_rates sound_rate;
extern BOOLE sound_stereo;
extern BOOLE sound_16bits;
extern BOOLE sound_wav_capture;
extern ULO sound_buffer_length;
extern sound_filters sound_filter;
extern sound_notifications sound_notification;


/*==================================*/
/* Property access                  */
/*==================================*/

extern void soundSetRate(sound_rates rate);
extern sound_rates soundGetRate(void);
extern ULO soundGetRateReal(void);
extern void soundSetStereo(BOOLE stereo);
extern BOOLE soundGetStereo(void);
extern void soundSet16Bits(BOOLE bits16);
extern BOOLE soundGet16Bits(void);
extern void soundSetEmulation(sound_emulations emulation);
extern sound_emulations soundGetEmulation(void);
extern void soundSetFilter(sound_filters filter);
extern sound_filters soundGetFilter(void);
extern void soundSetBufferLength(ULO ms);
extern ULO soundGetBufferLength(void);
extern void soundSetVolume(ULO volume);
extern ULO soundGetVolume(void);
extern void soundSetWAVDump(BOOLE wav_capture);
extern BOOLE soundGetWAVDump(void);
extern void soundSetNotification(sound_notifications notification);
extern sound_notifications soundGetNotification(void);

extern void soundEndOfLine(void); /* for bus.c */
extern void soundChannelKill(ULO ch); /* for wdmacon */
extern void soundChannelEnable(ULO ch); /* for wdmacon */

extern void soundState0(ULO ch); /* for wdbg.c */
extern void soundState1(ULO ch);
extern void soundState2(ULO ch);
extern void soundState3(ULO ch);
extern void soundState4(ULO ch);
extern void soundState5(ULO ch);


/*==================================*/
/* Standard Fellow Module Functions */
/*==================================*/

typedef void (*soundStateFunc)(ULO);

extern void soundEndOfLine(void);
extern void soundEndOfFrame(void);
extern void soundHardReset(void);
extern void soundEmulationStart(void);
extern void soundEmulationStop(void);
extern BOOLE soundStartup(void);
extern void soundShutdown(void);

#endif



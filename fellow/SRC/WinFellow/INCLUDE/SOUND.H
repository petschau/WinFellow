#pragma once

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
  uint32_t rates_max[2][2]; /* Maximum playback rate for [8/16 bits][mono/stereo] */
} sound_device;


extern sound_emulations sound_emulation;
extern sound_rates sound_rate;
extern bool sound_stereo;
extern bool sound_16bits;
extern BOOLE sound_wav_capture;
extern uint32_t sound_buffer_length;
extern sound_filters sound_filter;
extern sound_notifications sound_notification;


/*==================================*/
/* Property access                  */
/*==================================*/

extern void soundSetRate(sound_rates rate);
extern sound_rates soundGetRate();
extern uint32_t soundGetRateReal();
extern void soundSetStereo(bool stereo);
extern bool soundGetStereo();
extern void soundSet16Bits(bool bits16);
extern bool soundGet16Bits();
extern void soundSetEmulation(sound_emulations emulation);
extern sound_emulations soundGetEmulation();
extern void soundSetFilter(sound_filters filter);
extern sound_filters soundGetFilter();
extern void soundSetBufferLength(uint32_t ms);
extern uint32_t soundGetBufferLength();
extern void soundSetVolume(uint32_t volume);
extern uint32_t soundGetVolume();
extern void soundSetWAVDump(BOOLE wav_capture);
extern BOOLE soundGetWAVDump();
extern void soundSetNotification(sound_notifications notification);
extern sound_notifications soundGetNotification();

extern void soundEndOfLine(); /* for bus.c */
extern void soundChannelKill(uint32_t ch); /* for wdmacon */
extern void soundChannelEnable(uint32_t ch); /* for wdmacon */

extern void soundState0(uint32_t ch); /* for wdbg.c */
extern void soundState1(uint32_t ch);
extern void soundState2(uint32_t ch);
extern void soundState3(uint32_t ch);
extern void soundState4(uint32_t ch);
extern void soundState5(uint32_t ch);


/*==================================*/
/* Standard Fellow Module Functions */
/*==================================*/

typedef void (*soundStateFunc)(uint32_t);

extern void soundEndOfLine();
extern void soundHardReset();
extern void soundEmulationStart();
extern void soundEmulationStop();
extern BOOLE soundStartup();
extern void soundShutdown();

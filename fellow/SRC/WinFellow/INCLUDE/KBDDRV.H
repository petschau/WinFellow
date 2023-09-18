#pragma once

#include "KEYCODES.H"
#include "KBD.H"

#define MAX_KEYS	256
#define MAX_JOYKEY_VALUE	8

extern kbd_drv_pc_symbol kbddrv_DIK_to_symbol[MAX_KEYS];

extern char *symbol_pretty_name[106];

extern char *kbdDrvKeyString(uint32_t symbolickey);
extern char *kbdDrvKeyPrettyString(uint32_t symbolickey);
extern void kbdDrvJoystickReplacementSet(kbd_event event, uint32_t symbolickey);
extern uint32_t  kbdDrvJoystickReplacementGet(kbd_event event);
extern void kbdDrvHardReset();
extern void kbdDrvEmulationStart();
extern void kbdDrvEmulationStop();
extern void kbdDrvKeypressRaw(uint32_t, BOOL);
extern void kbdDrvStartup();
extern void kbdDrvShutdown();
extern void kbdDrvStateHasChanged(BOOLE);
extern void kbdDrvKeypressHandler();

#ifdef RETRO_PLATFORM
extern void kbdDrvEOFHandler();
extern void kbdDrvSetJoyKeyEnabled(uint32_t, uint32_t, BOOLE);
#endif

#pragma once

#include "fellow/api/defs.h"
#include "fellow/chipset/Keycodes.h"
#include "fellow/chipset/Kbd.h"

constexpr unsigned int MAX_KEYS = 256;
constexpr unsigned int MAX_JOYKEY_VALUE = 8;

extern kbd_drv_pc_symbol kbddrv_DIK_to_symbol[MAX_KEYS];

extern STR *symbol_pretty_name[kbd_drv_pc_symbol::PCK_LAST_KEY];

extern STR *kbdDrvKeyString(kbd_drv_pc_symbol symbolickey);
extern STR *kbdDrvKeyPrettyString(kbd_drv_pc_symbol symbolickey);
extern void kbdDrvJoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey);
extern kbd_drv_pc_symbol kbdDrvJoystickReplacementGet(kbd_event event);
extern void kbdDrvHardReset();
extern void kbdDrvEmulationStart();
extern void kbdDrvEmulationStop();
extern void kbdDrvKeypressRaw(ULO, BOOLE);
extern void kbdDrvStartup();
extern void kbdDrvShutdown();
extern void kbdDrvStateHasChanged(BOOLE);
extern void kbdDrvKeypressHandler();

#ifdef RETRO_PLATFORM
extern void kbdDrvEOFHandler();
extern void kbdDrvSetJoyKeyEnabled(ULO, ULO, BOOLE);
#endif

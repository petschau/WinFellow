#ifndef KBDDRV_H
#define KBDDRV_H

#include "KEYCODES.H"
#include "KBD.H"

#define MAX_KEYS	256
#define MAX_JOYKEY_VALUE	8

extern kbd_drv_pc_symbol kbddrv_DIK_to_symbol[MAX_KEYS];

extern STR *symbol_pretty_name[106];

extern STR *kbdDrvKeyString(uint32_t symbolickey);
extern STR *kbdDrvKeyPrettyString(uint32_t symbolickey);
extern void kbdDrvJoystickReplacementSet(kbd_event event, uint32_t symbolickey);
extern uint32_t  kbdDrvJoystickReplacementGet(kbd_event event);
extern void kbdDrvHardReset(void);
extern void kbdDrvEmulationStart(void);
extern void kbdDrvEmulationStop(void);
extern void kbdDrvKeypressRaw(uint32_t, BOOL);
extern void kbdDrvStartup(void);
extern void kbdDrvShutdown(void);
extern void kbdDrvStateHasChanged(BOOLE);
extern void kbdDrvKeypressHandler(void);

#ifdef RETRO_PLATFORM
extern void kbdDrvEOFHandler(void);
extern void kbdDrvSetJoyKeyEnabled(uint32_t, uint32_t, BOOLE);
#endif

#endif

#ifndef KBDDRV_H
#define KBDDRV_H

#define MAX_JOYKEY_VALUE	8

extern STR *symbol_pretty_name[106];

extern STR *kbdDrvKeyString(ULO symbolickey);
extern STR *kbdDrvKeyPrettyString(ULO symbolickey);
extern ULO kbdDrvCaptureKey(void);
extern void kbdDrvJoyReplacementSet(kbd_event event, ULO symbolickey);
extern ULO kbdDrvJoyReplacementGet(kbd_event event);
extern void kbdDrvHardReset(void);
extern void kbdDrvEmulationStart(void);
extern void kbdDrvEmulationStop(void);
extern void kbdDrvStartup(void);
extern void kbdDrvShutdown(void);

#endif

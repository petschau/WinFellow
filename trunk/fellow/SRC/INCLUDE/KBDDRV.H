#ifndef KBDDRV_H
#define KBDDRV_H


extern STR *kbdDrvKeyString(ULO symbolickey);
extern ULO kbdDrvCaptureKey(void);
extern void kbdDrvJoyReplacementSet(kbd_event event, ULO symbolickey);
extern ULO kbdDrvJoyReplacementGet(kbd_event event);
extern void kbdDrvHardReset(void);
extern void kbdDrvEmulationStart(void);
extern void kbdDrvEmulationStop(void);
extern void kbdDrvStartup(void);
extern void kbdDrvShutdown(void);

#endif

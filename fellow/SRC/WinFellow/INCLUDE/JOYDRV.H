#ifndef JOYDRV_H
#define JOYDRV_H

extern void joyDrvHardReset(void);
extern void joyDrvEmulationStart(void);
extern void joyDrvEmulationStop(void);
extern void joyDrvStartup(void);
extern void joyDrvShutdown(void);
extern void joyDrvStateHasChanged(BOOLE);
extern void joyDrvToggleFocus(void);
extern void joyDrvMovementHandler(void);

#endif
#ifndef MOUSEDRV_H
#define MOUSEDRV_H

extern void mouseDrvHardReset(void);
extern BOOLE mouseDrvEmulationStart(void);
extern void mouseDrvEmulationStop(void);
extern void mouseDrvStartup(void);
extern void mouseDrvShutdown(void);
extern void mouseDrvStateHasChanged(BOOLE);
extern void mouseDrvToggleFocus(void);
extern void mouseDrvMovementHandler(void);

#endif

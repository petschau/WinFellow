#pragma once

extern void joyDrvHardReset();
extern void joyDrvEmulationStart();
extern void joyDrvEmulationStop();
extern void joyDrvStartup();
extern void joyDrvShutdown();
extern void joyDrvStateHasChanged(BOOLE);
extern void joyDrvToggleFocus();
extern void joyDrvMovementHandler();

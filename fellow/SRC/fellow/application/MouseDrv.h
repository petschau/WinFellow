#pragma once

extern void mouseDrvHardReset();
extern BOOLE mouseDrvEmulationStart();
extern void mouseDrvEmulationStop();
extern BOOLE mouseDrvGetFocus();
extern void mouseDrvStartup();
extern void mouseDrvShutdown();
extern void mouseDrvStateHasChanged(BOOLE);
extern void mouseDrvToggleFocus();
extern void mouseDrvMovementHandler();
extern void mouseDrvSetFocus(const BOOLE, const BOOLE);

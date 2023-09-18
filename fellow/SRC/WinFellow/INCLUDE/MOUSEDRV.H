#pragma once

extern void  mouseDrvHardReset(void);
extern BOOLE mouseDrvEmulationStart(void);
extern void  mouseDrvEmulationStop(void);
extern BOOLE mouseDrvGetFocus(void);
extern void  mouseDrvStartup(void);
extern void  mouseDrvShutdown(void);
extern void  mouseDrvStateHasChanged(BOOLE);
extern void  mouseDrvToggleFocus(void);
extern void  mouseDrvMovementHandler(void);
extern void  mouseDrvSetFocus(const BOOLE, const BOOLE);

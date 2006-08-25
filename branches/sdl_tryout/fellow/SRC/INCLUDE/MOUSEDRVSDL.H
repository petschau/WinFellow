#ifndef MOUSEDRVSDL_H
#define MOUSEDRVSDL_H

extern void  mouseDrvSDLHardReset(void);
extern BOOLE mouseDrvSDLEmulationStart(void);
extern void  mouseDrvSDLEmulationStop(void);
extern void  mouseDrvSDLStartup(void);
extern void  mouseDrvSDLShutdown(void);
extern void  mouseDrvSDLStateHasChanged(BOOLE);
extern void  mouseDrvSDLToggleFocus(void);
extern void  mouseDrvSDLMovementHandler(SDL_Event * event);

#endif

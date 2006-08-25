#ifndef KBDDRVSDL_H
#define KBDDRVSDL_H

#define KBDDRVSDL_MAX_JOYKEY_VALUE	8

#include "SDL.h"

extern STR *kbdDrvSDLKeyString(SDLKey key);
extern STR *kbdDrvSDLKeyPrettyString(SDLKey key);
extern void kbdDrvSDLJoystickReplacementSet(kbd_event event, ULO symbolickey);
extern ULO  kbdDrvSDLJoystickReplacementGet(kbd_event event);
extern void kbdDrvSDLHardReset(void);
extern void kbdDrvSDLEmulationStart(void);
extern void kbdDrvSDLEmulationStop(void);
extern void kbdDrvSDLStartup(void);
extern void kbdDrvSDLShutdown(void);
extern void kbdDrvStateHasChanged(BOOLE);
extern void kbdDrvSDLKeypressHandler(SDL_Event* event);

#endif

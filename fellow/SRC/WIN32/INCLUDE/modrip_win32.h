/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* OS-dependant parts of the module ripper - Windows GUI code                 */
/* Author: Torsten Enderling (carfesh@gmx.net)                                */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#ifndef _MODRIP_WIN32_H_
#define _MODRIP_WIN32_H_

/* fellow includes */
#include "defs.h"

/* own includes */
#include "modrip.h"

extern BOOLE modripGuiInitialize(void);
extern void modripGuiSetBusy(void);
extern void modripGuiUnSetBusy(void);
extern BOOLE modripGuiSaveRequest(struct ModuleInfo *);
extern void modripGuiErrorSave(struct ModuleInfo *);
extern BOOLE modripGuiRipFloppy(int);
extern BOOLE modripGuiRipMemory(void);
extern void modripGuiUnInitialize(void);
extern void modripGuiError(char *);

#endif
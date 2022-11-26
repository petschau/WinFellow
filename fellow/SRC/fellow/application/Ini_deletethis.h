#pragma once

#include "fellow/api/defs.h"

/*============================================================================*/
/* struct that holds initialization data                                      */
/*============================================================================*/

extern ini *wgui_ini;

/*============================================================================*/
/* struct ini property access functions                                       */
/*============================================================================*/

/*============================================================================*/
/* struct iniManager                                                          */
/*============================================================================*/

struct iniManager
{
  ini *m_current_ini;
  ini *m_default_ini;
};

/*============================================================================*/
/* struct iniManager property access functions                                */
/*============================================================================*/

extern void iniManagerSetCurrentInitdata(iniManager *initdatamanager, ini *currentinitdata);
extern ini *iniManagerGetCurrentInitdata(iniManager *initdatamanager);
extern void iniManagerSetDefaultInitdata(iniManager *inimanager, ini *initdata);
extern ini *iniManagerGetDefaultInitdata(iniManager *inimanager);

/*============================================================================*/
/* struct iniManager utility functions                                        */
/*============================================================================*/

extern void iniManagerStartup(iniManager *initdatamanager);
extern void iniManagerShutdown(iniManager *initdatamanager);

/*============================================================================*/
/* The actual iniManager instance                                             */
/*============================================================================*/

extern iniManager ini_manager;

extern void iniEmulationStart();
extern void iniEmulationStop();
extern void iniStartup();
extern void iniShutdown();

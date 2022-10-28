#pragma once

#include "fellow/api/defs.h"

/*============================================================================*/
/* struct that holds initialization data                                      */
/*============================================================================*/

struct ini
{

  STR m_description[256];

  /*==========================================================================*/
  /* Holds current used configuration filename                                */
  /*==========================================================================*/

  STR m_current_configuration[CFG_FILENAME_LENGTH];

  /*==========================================================================*/
  /* Window positions (Main Window, Emulation Window)                         */
  /*==========================================================================*/

  int m_mainwindowxposition;
  int m_mainwindowyposition;
  int m_emulationwindowxposition;
  int m_emulationwindowyposition;

  /*==========================================================================*/
  /* History of used config files                                             */
  /*==========================================================================*/

  STR m_configuration_history[4][CFG_FILENAME_LENGTH];

  /*==========================================================================*/
  /* Holds last used directories                                              */
  /*==========================================================================*/

  STR m_lastusedkeydir[CFG_FILENAME_LENGTH];
  STR m_lastusedkickimagedir[CFG_FILENAME_LENGTH];
  STR m_lastusedconfigurationdir[CFG_FILENAME_LENGTH];
  ULO m_lastusedconfigurationtab;
  STR m_lastusedglobaldiskdir[CFG_FILENAME_LENGTH];
  STR m_lastusedhdfdir[CFG_FILENAME_LENGTH];
  STR m_lastusedmoddir[CFG_FILENAME_LENGTH];
  STR m_lastusedstatefiledir[CFG_FILENAME_LENGTH];
  STR m_lastusedpresetromdir[CFG_FILENAME_LENGTH];

  /*==========================================================================*/
  /* pause emulation when window loses focus                                  */
  /*==========================================================================*/
  BOOLE m_pauseemulationwhenwindowlosesfocus;

  //==========================================================================
  // Debug options for graphics emulation, may change from release to release
  //==========================================================================
  bool m_gfxDebugImmediateRendering;
};

extern ini *wgui_ini;

/*============================================================================*/
/* struct ini property access functions                                       */
/*============================================================================*/

extern int iniGetMainWindowXPos(ini *);
extern int iniGetMainWindowYPos(ini *);
extern int iniGetEmulationWindowXPos(ini *);
extern int iniGetEmulationWindowYPos(ini *);

extern void iniSetMainWindowPosition(ini *initdata, ULO mainwindowxpos, ULO mainwindowypos);
extern void iniSetEmulationWindowPosition(ini *initdata, ULO emulationwindowxpos, ULO emulationwindowypos);
extern STR *iniGetConfigurationHistoryFilename(ini *initdata, ULO index);
extern void iniSetConfigurationHistoryFilename(ini *initdata, ULO index, const char *cfgfilename);
extern STR *iniGetCurrentConfigurationFilename(ini *initdata);
extern void iniSetCurrentConfigurationFilename(ini *initdata, STR *configuration);
extern void iniSetLastUsedCfgDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedCfgDir(ini *initdata);
extern void iniSetLastUsedKickImageDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedKickImageDir(ini *initdata);
extern void iniSetLastUsedKeyDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedKeyDir(ini *initdata);
extern void iniSetLastUsedGlobalDiskDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedGlobalDiskDir(ini *initdata);
extern void iniSetLastUsedHdfDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedHdfDir(ini *initdata);
extern void iniSetLastUsedModDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedModDir(ini *initdata);
extern void iniSetLastUsedCfgTab(ini *initdata, ULO cfgTab);
extern ULO iniGetLastUsedCfgTab(ini *initdata);
extern void iniSetLastUsedStateFileDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedStateFileDir(ini *initdata);
extern void iniSetLastUsedPresetROMDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedPresetROMDir(ini *initdata);
extern BOOLE iniGetPauseEmulationWhenWindowLosesFocus(ini *initdata);
extern void iniSetPauseEmulationWhenWindowLosesFocus(ini *initdata, BOOLE pause);

extern bool iniGetGfxDebugImmediateRendering(ini *initdata);
extern void iniSetGfxDebugImmediateRendering(ini *initdata, bool gfxDebugImmediateRendering);

extern BOOLE iniSetOption(ini *initdata, STR *initoptionstr);
extern BOOLE iniSaveOptions(ini *initdata, FILE *inifile);

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

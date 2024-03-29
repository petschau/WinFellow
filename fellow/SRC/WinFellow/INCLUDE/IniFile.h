#pragma once

#include <cstdint>
#include "Defs.h"

/*============================================================================*/
/* struct that holds initialization data                                      */
/*============================================================================*/

struct ini
{

  char m_description[256];

  /*==========================================================================*/
  /* Holds current used configuration filename                                */
  /*==========================================================================*/

  char m_current_configuration[CFG_FILENAME_LENGTH];

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

  char m_configuration_history[4][CFG_FILENAME_LENGTH];

  /*==========================================================================*/
  /* Holds last used directories                                              */
  /*==========================================================================*/

  char m_lastusedkeydir[CFG_FILENAME_LENGTH];
  char m_lastusedkickimagedir[CFG_FILENAME_LENGTH];
  char m_lastusedconfigurationdir[CFG_FILENAME_LENGTH];
  uint32_t m_lastusedconfigurationtab;
  char m_lastusedglobaldiskdir[CFG_FILENAME_LENGTH];
  char m_lastusedhdfdir[CFG_FILENAME_LENGTH];
  char m_lastusedmoddir[CFG_FILENAME_LENGTH];
  char m_lastusedstatefiledir[CFG_FILENAME_LENGTH];
  char m_lastusedpresetromdir[CFG_FILENAME_LENGTH];

  /*==========================================================================*/
  /* pause emulation when window loses focus                                  */
  /*==========================================================================*/
  BOOLE m_pauseemulationwhenwindowlosesfocus;
};

extern ini *wgui_ini;

/*============================================================================*/
/* struct ini property access functions                                       */
/*============================================================================*/

extern int iniGetMainWindowXPos(ini *);
extern int iniGetMainWindowYPos(ini *);
extern int iniGetEmulationWindowXPos(ini *);
extern int iniGetEmulationWindowYPos(ini *);

extern void iniSetMainWindowPosition(ini *initdata, uint32_t mainwindowxpos, uint32_t mainwindowypos);
extern void iniSetEmulationWindowPosition(ini *initdata, uint32_t emulationwindowxpos, uint32_t emulationwindowypos);
extern const char *iniGetConfigurationHistoryFilename(ini *initdata, uint32_t position);
extern void iniSetConfigurationHistoryFilename(ini *initdata, uint32_t position, const char *configuration);
extern const char *iniGetCurrentConfigurationFilename(ini *initdata);
extern void iniSetCurrentConfigurationFilename(ini *initdata, const char *configuration);
extern void iniSetLastUsedCfgDir(ini *initdata, const char *directory);
extern const char *iniGetLastUsedCfgDir(ini *initdata);
extern void iniSetLastUsedKickImageDir(ini *initdata, const char *directory);
extern const char *iniGetLastUsedKickImageDir(ini *initdata);
extern void iniSetLastUsedKeyDir(ini *initdata, const char *directory);
extern const char *iniGetLastUsedKeyDir(ini *initdata);
extern void iniSetLastUsedGlobalDiskDir(ini *initdata, const char *directory);
extern const char *iniGetLastUsedGlobalDiskDir(ini *initdata);
extern void iniSetLastUsedHdfDir(ini *initdata, const char *directory);
extern const char *iniGetLastUsedHdfDir(ini *initdata);
extern void iniSetLastUsedModDir(ini *initdata, const char *directory);
extern const char *iniGetLastUsedModDir(ini *initdata);
extern void iniSetLastUsedCfgTab(ini *initdata, uint32_t cfgTab);
extern uint32_t iniGetLastUsedCfgTab(ini *initdata);
extern void iniSetLastUsedStateFileDir(ini *initdata, const char *directory);
extern const char *iniGetLastUsedStateFileDir(ini *initdata);
extern void iniSetLastUsedPresetROMDir(ini *initdata, const char *directory);
extern const char *iniGetLastUsedPresetROMDir(ini *initdata);
extern BOOLE iniGetPauseEmulationWhenWindowLosesFocus(ini *initdata);
extern void iniSetPauseEmulationWhenWindowLosesFocus(ini *initdata, BOOLE pause);

extern BOOLE iniSetOption(ini *initdata, char *initoptionstr);
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

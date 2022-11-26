#pragma once

#include <cstdio>

#include "fellow/api/defs.h"

class IniValues
{
private:
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

public:
  void SetDescription(STR *description);
  STR *GetDescription();

  void SetMainWindowXPos(int xpos);
  int GetMainWindowXPos();
  void SetMainWindowYPos(int ypos);
  int GetMainWindowYPos();
  void SetMainWindowPosition(ULO mainwindowxpos, ULO mainwindowypos);

  void SetEmulationWindowXPos(int xpos);
  int GetEmulationWindowXPos();
  void SetEmulationWindowYPos(int ypos);
  int GetEmulationWindowYPos();
  void SetEmulationWindowPosition(ULO emulationwindowxpos, ULO emulationwindowypos);

  void SetConfigurationHistoryFilename(ULO index, const char *cfgfilename);
  STR *GetConfigurationHistoryFilename(ULO index);

  void SetCurrentConfigurationFilename(STR *configuration);
  STR *GetCurrentConfigurationFilename();

  void SetLastUsedCfgDir(STR *directory);
  STR *GetLastUsedCfgDir();

  void SetLastUsedCfgTab(ULO cfgTab);
  ULO GetLastUsedCfgTab();

  void SetLastUsedStateFileDir(STR *directory);
  STR *GetLastUsedStateFileDir();

  void SetLastUsedPresetROMDir(STR *directory);
  STR *GetLastUsedPresetROMDir();

  void SetLastUsedKickImageDir(STR *directory);
  STR *GetLastUsedKickImageDir();

  void SetLastUsedKeyDir(STR *directory);
  STR *GetLastUsedKeyDir();

  void SetLastUsedGlobalDiskDir(STR *directory);
  STR *GetLastUsedGlobalDiskDir();

  void SetLastUsedHdfDir(STR *directory);
  STR *GetLastUsedHdfDir();

  void SetLastUsedModDir(STR *directory);
  STR *GetLastUsedModDir();

  BOOLE GetPauseEmulationWhenWindowLosesFocus();
  void SetPauseEmulationWhenWindowLosesFocus(BOOLE pause);

  bool GetGfxDebugImmediateRendering();
  void SetGfxDebugImmediateRendering(bool gfxDebugImmediateRendering);

  void SetDefaults();

  BOOLE SetOption(STR *initoptionstr);
  BOOLE SaveOptions(FILE *inifile);
};

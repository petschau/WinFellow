#pragma once

#include <cstdio>
#include <string>

#include "fellow/api/defs.h"

class IniValues
{
private:
  std::string _description;

  /*==========================================================================*/
  /* Holds current used configuration filename                                */
  /*==========================================================================*/

  std::string _currentConfiguration;

  /*==========================================================================*/
  /* Window positions (Main Window, Emulation Window)                         */
  /*==========================================================================*/

  int _mainwindowxposition;
  int _mainwindowyposition;
  int _emulationwindowxposition;
  int _emulationwindowyposition;

  /*==========================================================================*/
  /* History of used config files                                             */
  /*==========================================================================*/

  std::string _configurationHistory[4];

  /*==========================================================================*/
  /* Holds last used directories                                              */
  /*==========================================================================*/

  std::string _lastusedkeydir;
  std::string _lastusedkickimagedir;
  std::string _lastusedconfigurationdir;
  ULO _lastusedconfigurationtab;
  std::string _lastusedglobaldiskdir;
  std::string _lastusedhdfdir;
  std::string _lastusedmoddir;
  std::string _lastusedstatefiledir;
  std::string _lastusedpresetromdir;

  bool _pauseemulationwhenwindowlosesfocus;

  //=========================================================================
  // Debug options for graphics emulation, may change from release to release
  //=========================================================================
  bool _gfxDebugImmediateRendering;

public:
  void SetDescription(const std::string &description);
  const std::string &GetDescription() const;

  void SetMainWindowXPos(int xpos);
  int GetMainWindowXPos() const;
  void SetMainWindowYPos(int ypos);
  int GetMainWindowYPos() const;
  void SetMainWindowPosition(ULO mainwindowxpos, ULO mainwindowypos);

  void SetEmulationWindowXPos(int xpos);
  int GetEmulationWindowXPos() const;
  void SetEmulationWindowYPos(int ypos);
  int GetEmulationWindowYPos() const;
  void SetEmulationWindowPosition(ULO emulationwindowxpos, ULO emulationwindowypos);

  void SetConfigurationHistoryFilename(ULO index, const std::string &cfgfilename);
  const std::string &GetConfigurationHistoryFilename(ULO index) const;

  void SetCurrentConfigurationFilename(const std::string &configuration);
  const std::string &GetCurrentConfigurationFilename() const;

  void SetLastUsedCfgDir(const std::string &directory);
  const std::string &GetLastUsedCfgDir() const;

  void SetLastUsedCfgTab(ULO cfgTab);
  ULO GetLastUsedCfgTab() const;

  void SetLastUsedStateFileDir(const std::string &directory);
  const std::string &GetLastUsedStateFileDir() const;

  void SetLastUsedPresetROMDir(const std::string &directory);
  const std::string &GetLastUsedPresetROMDir() const;

  void SetLastUsedKickImageDir(const std::string &directory);
  const std::string &GetLastUsedKickImageDir() const;

  void SetLastUsedKeyDir(const std::string &directory);
  const std::string &GetLastUsedKeyDir() const;

  void SetLastUsedGlobalDiskDir(const std::string &directory);
  const std::string &GetLastUsedGlobalDiskDir() const;

  void SetLastUsedHdfDir(const std::string &directory);
  const std::string &GetLastUsedHdfDir() const;

  void SetLastUsedModDir(const std::string &directory);
  const std::string &GetLastUsedModDir() const;

  void SetPauseEmulationWhenWindowLosesFocus(bool pause);
  bool GetPauseEmulationWhenWindowLosesFocus() const;

  void SetGfxDebugImmediateRendering(bool gfxDebugImmediateRendering);
  bool GetGfxDebugImmediateRendering() const;

  void SetDefaults();
};

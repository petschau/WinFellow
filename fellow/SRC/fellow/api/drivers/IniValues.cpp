#include "fellow/api/drivers/IniValues.h"
#include "versioninfo.h"
#include "fellow/api/Services.h"

using namespace fellow::api;
using namespace std;

void IniValues::SetDescription(const string &description)
{
  _description = description;
}

const string &IniValues::GetDescription() const
{
  return _description;
}

void IniValues::SetMainWindowXPos(int xpos)
{
  _mainwindowxposition = xpos;
}

int IniValues::GetMainWindowXPos() const
{
  return _mainwindowxposition;
}

void IniValues::SetMainWindowYPos(int ypos)
{
  _mainwindowyposition = ypos;
}

int IniValues::GetMainWindowYPos() const
{
  return _mainwindowyposition;
}

void IniValues::SetMainWindowPosition(ULO mainwindowxpos, ULO mainwindowypos)
{
  SetMainWindowXPos(mainwindowxpos);
  SetMainWindowYPos(mainwindowypos);
}

void IniValues::SetEmulationWindowXPos(int xpos)
{
  _emulationwindowxposition = xpos;
}

int IniValues::GetEmulationWindowXPos() const
{
  return _emulationwindowxposition;
}

void IniValues::SetEmulationWindowYPos(int ypos)
{
  _emulationwindowyposition = ypos;
}

int IniValues::GetEmulationWindowYPos() const
{
  return _emulationwindowyposition;
}

void IniValues::SetEmulationWindowPosition(ULO emulationwindowxpos, ULO emulationwindowypos)
{
  SetEmulationWindowXPos(emulationwindowxpos);
  SetEmulationWindowYPos(emulationwindowypos);
}

void IniValues::SetConfigurationHistoryFilename(ULO index, const string &cfgfilename)
{
  _configurationHistory[index] = cfgfilename;
}

const string &IniValues::GetConfigurationHistoryFilename(ULO index) const
{
  return _configurationHistory[index];
}

void IniValues::SetCurrentConfigurationFilename(const string &configuration)
{
  _currentConfiguration = configuration;
}

const string &IniValues::GetCurrentConfigurationFilename() const
{
  return _currentConfiguration;
}

void IniValues::SetLastUsedCfgDir(const string &directory)
{
  _lastusedconfigurationdir = directory;
}

const string &IniValues::GetLastUsedCfgDir() const
{
  return _lastusedconfigurationdir;
}

void IniValues::SetLastUsedCfgTab(ULO cfgTab)
{
  _lastusedconfigurationtab = cfgTab;
}

ULO IniValues::GetLastUsedCfgTab() const
{
  return _lastusedconfigurationtab;
}

void IniValues::SetLastUsedStateFileDir(const string &directory)
{
  _lastusedstatefiledir = directory;
}

const string &IniValues::GetLastUsedStateFileDir() const
{
  return _lastusedstatefiledir;
}

void IniValues::SetLastUsedPresetROMDir(const string &directory)
{
  _lastusedpresetromdir = directory;
}

const string &IniValues::GetLastUsedPresetROMDir() const
{
  return _lastusedpresetromdir;
}

void IniValues::SetLastUsedKickImageDir(const string &directory)
{
  _lastusedkickimagedir = directory;
}

const string &IniValues::GetLastUsedKickImageDir() const
{
  return _lastusedkickimagedir;
}

void IniValues::SetLastUsedKeyDir(const string &directory)
{
  _lastusedkeydir = directory;
}

const string &IniValues::GetLastUsedKeyDir() const
{
  return _lastusedkeydir;
}

void IniValues::SetLastUsedGlobalDiskDir(const string &directory)
{
  _lastusedglobaldiskdir = directory;
}

const string &IniValues::GetLastUsedGlobalDiskDir() const
{
  return _lastusedglobaldiskdir;
}

void IniValues::SetLastUsedHdfDir(const string &directory)
{
  _lastusedhdfdir = directory;
}

const string &IniValues::GetLastUsedHdfDir() const
{
  return _lastusedhdfdir;
}

void IniValues::SetLastUsedModDir(const string &directory)
{
  _lastusedmoddir = directory;
}

const string &IniValues::GetLastUsedModDir() const
{
  return _lastusedmoddir;
}

void IniValues::SetPauseEmulationWhenWindowLosesFocus(bool pause)
{
  _pauseemulationwhenwindowlosesfocus = pause;
}

bool IniValues::GetPauseEmulationWhenWindowLosesFocus() const
{
  return _pauseemulationwhenwindowlosesfocus;
}

void IniValues::SetGfxDebugImmediateRendering(bool gfxDebugImmediateRendering)
{
  _gfxDebugImmediateRendering = gfxDebugImmediateRendering;
}

bool IniValues::GetGfxDebugImmediateRendering() const
{
  return _gfxDebugImmediateRendering;
}

void IniValues::SetDefaults()
{
  SetDescription(FELLOWLONGVERSION);

  SetMainWindowXPos(0);
  SetMainWindowYPos(0);

  SetEmulationWindowXPos(0);
  SetEmulationWindowYPos(0);

  for (ULO i = 0; i < 4; i++)
  {
    SetConfigurationHistoryFilename(i, "");
  }

  STR defaultConfigFilename[CFG_FILENAME_LENGTH];
  Service->Fileops.GetDefaultConfigFileName(defaultConfigFilename);
  SetCurrentConfigurationFilename(defaultConfigFilename);

  SetLastUsedCfgDir("");
  SetLastUsedCfgTab(0);
  SetLastUsedKickImageDir("");
  SetLastUsedKeyDir("");
  SetLastUsedGlobalDiskDir("");
  SetLastUsedHdfDir("");
  SetLastUsedModDir("");
  SetLastUsedStateFileDir("");
  SetLastUsedPresetROMDir("");
  SetPauseEmulationWhenWindowLosesFocus(TRUE);

  SetGfxDebugImmediateRendering(false);
}

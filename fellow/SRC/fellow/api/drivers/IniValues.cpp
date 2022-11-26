#include "fellow/api/drivers/IniValues.h"

#include "fellow/api/Services.h"

using namespace fellow::api;

void IniValues::SetDescription(STR *description)
{
  strcpy(m_description, description);
}

STR *IniValues::GetDescription()
{
  return m_description;
}

void IniValues::SetMainWindowXPos(int xpos)
{
  m_mainwindowxposition = xpos;
}

int IniValues::GetMainWindowXPos()
{
  return m_mainwindowxposition;
}

void IniValues::SetMainWindowYPos(int ypos)
{
  m_mainwindowyposition = ypos;
}

int IniValues::GetMainWindowYPos()
{
  return m_mainwindowyposition;
}

void IniValues::SetMainWindowPosition(ULO mainwindowxpos, ULO mainwindowypos)
{
  SetMainWindowXPos(mainwindowxpos);
  SetMainWindowYPos(mainwindowypos);
}

void IniValues::SetEmulationWindowXPos(int xpos)
{
  m_emulationwindowxposition = xpos;
}

int IniValues::GetEmulationWindowXPos()
{
  return m_emulationwindowxposition;
}

void IniValues::SetEmulationWindowYPos(int ypos)
{
  m_emulationwindowyposition = ypos;
}

int IniValues::GetEmulationWindowYPos()
{
  return m_emulationwindowyposition;
}

void IniValues::SetEmulationWindowPosition(ULO emulationwindowxpos, ULO emulationwindowypos)
{
  SetEmulationWindowXPos(emulationwindowxpos);
  SetEmulationWindowYPos(emulationwindowypos);
}

void IniValues::SetConfigurationHistoryFilename(ULO index, const char *cfgfilename)
{
  strncpy(m_configuration_history[index], cfgfilename, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetConfigurationHistoryFilename(ULO index)
{
  return m_configuration_history[index];
}

void IniValues::SetCurrentConfigurationFilename(STR *configuration)
{
  strncpy(m_current_configuration, configuration, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetCurrentConfigurationFilename()
{
  return m_current_configuration;
}

void IniValues::SetLastUsedCfgDir(STR *directory)
{
  strncpy(m_lastusedconfigurationdir, directory, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetLastUsedCfgDir()
{
  return m_lastusedconfigurationdir;
}

void IniValues::SetLastUsedCfgTab(ULO cfgTab)
{
  m_lastusedconfigurationtab = cfgTab;
}

ULO IniValues::GetLastUsedCfgTab()
{
  return m_lastusedconfigurationtab;
}

void IniValues::SetLastUsedStateFileDir(STR *directory)
{
  strncpy(m_lastusedstatefiledir, directory, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetLastUsedStateFileDir()
{
  return m_lastusedstatefiledir;
}

void IniValues::SetLastUsedPresetROMDir(STR *directory)
{
  strncpy(m_lastusedpresetromdir, directory, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetLastUsedPresetROMDir()
{
  return m_lastusedpresetromdir;
}

void IniValues::SetLastUsedKickImageDir(STR *directory)
{
  strncpy(m_lastusedkickimagedir, directory, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetLastUsedKickImageDir()
{
  return m_lastusedkickimagedir;
}

void IniValues::SetLastUsedKeyDir(STR *directory)
{
  strncpy(m_lastusedkeydir, directory, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetLastUsedKeyDir()
{
  return m_lastusedkeydir;
}

void IniValues::SetLastUsedGlobalDiskDir(STR *directory)
{
  strncpy(m_lastusedglobaldiskdir, directory, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetLastUsedGlobalDiskDir()
{
  return m_lastusedglobaldiskdir;
}

void IniValues::SetLastUsedHdfDir(STR *directory)
{
  strncpy(m_lastusedhdfdir, directory, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetLastUsedHdfDir()
{
  return m_lastusedhdfdir;
}

void IniValues::SetLastUsedModDir(STR *directory)
{
  strncpy(m_lastusedmoddir, directory, CFG_FILENAME_LENGTH);
}

STR *IniValues::GetLastUsedModDir()
{
  return m_lastusedmoddir;
}

BOOLE IniValues::GetPauseEmulationWhenWindowLosesFocus()
{
  return m_pauseemulationwhenwindowlosesfocus;
}

void IniValues::SetPauseEmulationWhenWindowLosesFocus(BOOLE pause)
{
  m_pauseemulationwhenwindowlosesfocus = pause;
}

bool IniValues::GetGfxDebugImmediateRendering()
{
  return m_gfxDebugImmediateRendering;
}

void IniValues::SetGfxDebugImmediateRendering(bool gfxDebugImmediateRendering)
{
  m_gfxDebugImmediateRendering = gfxDebugImmediateRendering;
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

#include "fellow/os/null/IniDriver.h"
#include "fellow/api/Services.h"

using namespace fellow::api;
using namespace std;

ULO IniDriver::GetULOFromString(const string &value)
{
  return stoul(value);
}

int IniDriver::GetintFromString(const string &value)
{
  return stol(value);
}

BOOLE IniDriver::GetBOOLEFromString(const string &value)
{
  return stoul(value) == 1;
}

void IniDriver::StripTrailingNewlines(STR *line)
{
  size_t length = strlen(line);
  while ((length > 0) && ((line[length - 1] == '\n') || (line[length - 1] == '\r')))
    line[--length] = '\0';
}

/*============================================================================*/
/* Set initialization options                                                 */
/* Returns TRUE if the option was recognized                                  */
/*============================================================================*/

BOOLE IniDriver::SetOption(IniValues *initdata, const string &initoptionstr)
{
  size_t splitPosition = initoptionstr.find('=');
  if (splitPosition == string::npos)
  {
    return FALSE;
  }

  string option = initoptionstr.substr(0, splitPosition);
  string value = initoptionstr.substr(splitPosition + 1);

  /* Standard initialization options */

  if (option == "last_used_configuration")
  {
    if (value.empty())
    {
      // Config option was empty
      STR defaultConfigurationFilename[CFG_FILENAME_LENGTH];
      Service->Fileops.GetDefaultConfigFileName(defaultConfigurationFilename);
      initdata->SetCurrentConfigurationFilename(defaultConfigurationFilename);
    }
    else
    {
      fs_wrapper_object_info *fwoi = Service->FSWrapper.GetFSObjectInfo(value);
      if (fwoi == nullptr)
      {
        // Error, no-access etc.
        STR defaultConfigurationFilename[CFG_FILENAME_LENGTH];
        Service->Fileops.GetDefaultConfigFileName(defaultConfigurationFilename);
        initdata->SetCurrentConfigurationFilename(defaultConfigurationFilename);
      }
      else
      {
        // File appears ok
        initdata->SetCurrentConfigurationFilename(value);
        delete fwoi;
      }
    }
  }
  else if (option == "last_used_cfg_dir")
  {
    initdata->SetLastUsedCfgDir(value);
  }
  else if (option == "main_window_x_pos")
  {
    initdata->SetMainWindowXPos(GetintFromString(value));
  }
  else if (option == "main_window_y_pos")
  {
    initdata->SetMainWindowYPos(GetintFromString(value));
  }
  else if (option == "emu_window_x_pos")
  {
    initdata->SetEmulationWindowXPos(GetintFromString(value));
  }
  else if (option == "emu_window_y_pos")
  {
    initdata->SetEmulationWindowYPos(GetintFromString(value));
  }
  else if (option == "config_history_0")
  {
    initdata->SetConfigurationHistoryFilename(0, value);
  }
  else if (option == "config_history_1")
  {
    initdata->SetConfigurationHistoryFilename(1, value);
  }
  else if (option == "config_history_2")
  {
    initdata->SetConfigurationHistoryFilename(2, value);
  }
  else if (option == "config_history_3")
  {
    initdata->SetConfigurationHistoryFilename(3, value);
  }
  else if (option == "last_used_kick_image_dir")
  {
    initdata->SetLastUsedKickImageDir(value);
  }
  else if (option == "last_used_key_dir")
  {
    initdata->SetLastUsedKeyDir(value);
  }
  else if (option == "last_used_global_disk_dir")
  {
    initdata->SetLastUsedGlobalDiskDir(value);
  }
  else if (option == "last_used_hdf_dir")
  {
    initdata->SetLastUsedHdfDir(value);
  }
  else if (option == "last_used_mod_dir")
  {
    initdata->SetLastUsedModDir(value);
  }
  else if (option == "last_used_cfg_tab")
  {
    initdata->SetLastUsedCfgTab(GetULOFromString(value));
  }
  else if (option == "last_used_statefile_dir")
  {
    initdata->SetLastUsedStateFileDir(value);
  }
  else if (option == "last_used_preset_rom_dir")
  {
    initdata->SetLastUsedPresetROMDir(value);
  }
  else if (option == "pause_emulation_when_window_loses_focus")
  {
    initdata->SetPauseEmulationWhenWindowLosesFocus(value == "true");
  }
  else if (option == "gfx_debug_immediate_rendering")
  {
    initdata->SetGfxDebugImmediateRendering(value == "true");
  }
  else
    return FALSE;

  return TRUE;
}

BOOLE IniDriver::LoadIniFile(IniValues *initdata, FILE *inifile)
{
  char line[256];
  while (!feof(inifile))
  {
    fgets(line, 256, inifile);
    StripTrailingNewlines(line);
    SetOption(initdata, line);
  }
  return TRUE;
}

BOOLE IniDriver::LoadIniFromFilename(IniValues *inidata, STR *filename)
{
  FILE *inifile;
  BOOLE result;

  inifile = fopen(filename, "r");
  result = (inifile != NULL);
  if (result)
  {
    result = LoadIniFile(inidata, inifile);
    fclose(inifile);
  }
  return result;
}

BOOLE IniDriver::SaveToFile(IniValues *initdata, FILE *inifile)
{
  return SaveOptions(initdata, inifile);
}

BOOLE IniDriver::SaveToFilename(IniValues *initdata, const string &filename)
{
  FILE *inifile = fopen(filename.c_str(), "w");
  if (inifile == nullptr)
  {
    return FALSE;
  }

  BOOLE result = SaveToFile(initdata, inifile);
  fclose(inifile);

  return result;
}

BOOLE IniDriver::SaveOptions(IniValues *initdata, FILE *inifile)
{
  fprintf(inifile, "ini_description=%s\n", initdata->GetDescription().c_str());
  fprintf(inifile, "main_window_x_pos=%d\n", initdata->GetMainWindowXPos());
  fprintf(inifile, "main_window_y_pos=%d\n", initdata->GetMainWindowYPos());
  fprintf(inifile, "emu_window_x_pos=%d\n", initdata->GetEmulationWindowXPos());
  fprintf(inifile, "emu_window_y_pos=%d\n", initdata->GetEmulationWindowYPos());
  fprintf(inifile, "config_history_0=%s\n", initdata->GetConfigurationHistoryFilename(0).c_str());
  fprintf(inifile, "config_history_1=%s\n", initdata->GetConfigurationHistoryFilename(1).c_str());
  fprintf(inifile, "config_history_2=%s\n", initdata->GetConfigurationHistoryFilename(2).c_str());
  fprintf(inifile, "config_history_3=%s\n", initdata->GetConfigurationHistoryFilename(3).c_str());
  fprintf(inifile, "last_used_configuration=%s\n", initdata->GetCurrentConfigurationFilename().c_str());
  fprintf(inifile, "last_used_cfg_dir=%s\n", initdata->GetLastUsedCfgDir().c_str());
  fprintf(inifile, "last_used_cfg_tab=%u\n", initdata->GetLastUsedCfgTab());
  fprintf(inifile, "last_used_kick_image_dir=%s\n", initdata->GetLastUsedKickImageDir().c_str());
  fprintf(inifile, "last_used_key_dir=%s\n", initdata->GetLastUsedKeyDir().c_str());
  fprintf(inifile, "last_used_global_disk_dir=%s\n", initdata->GetLastUsedGlobalDiskDir().c_str());
  fprintf(inifile, "last_used_hdf_dir=%s\n", initdata->GetLastUsedHdfDir().c_str());
  fprintf(inifile, "last_used_mod_dir=%s\n", initdata->GetLastUsedModDir().c_str());
  fprintf(inifile, "last_used_statefile_dir=%s\n", initdata->GetLastUsedStateFileDir().c_str());
  fprintf(inifile, "last_used_preset_rom_dir=%s\n", initdata->GetLastUsedPresetROMDir().c_str());
  fprintf(inifile, "pause_emulation_when_window_loses_focus=%s\n", initdata->GetPauseEmulationWhenWindowLosesFocus() ? "true" : "false");
  fprintf(inifile, "gfx_debug_immediate_rendering=%s\n", initdata->GetGfxDebugImmediateRendering() ? "true" : "false");

  return TRUE;
}

void IniDriver::SetCurrentInitdata(IniValues *initdata)
{
  m_current_ini = initdata;
}

IniValues *IniDriver::GetCurrentInitdata()
{
  return m_current_ini;
}

void IniDriver::SetDefaultInitdata(IniValues *initdata)
{
  m_default_ini = initdata;
}

IniValues *IniDriver::GetDefaultInitdata()
{
  return m_default_ini;
}

IniValues *IniDriver::GetNewIni()
{
  IniValues *initdata = new IniValues();
  initdata->SetDefaults();
  return initdata;
}

void IniDriver::FreeIni(IniValues *initdata)
{
  free(initdata);
}

void IniDriver::EmulationStart()
{
}

void IniDriver::EmulationStop()
{
}

void IniDriver::Initialize()
{
  IniValues *initdata = GetNewIni();
  SetCurrentInitdata(initdata);

  // TODO: The only OS specific in this file was the MAX_PATH macro, find some other way around that
  STR defaultFilepath[CFG_FILENAME_LENGTH];
  Service->Fileops.GetGenericFileName(defaultFilepath, "WinFellow", INI_FILENAME);

  _defaultIniFilePath = defaultFilepath;

  if (LoadIniFromFilename(initdata, defaultFilepath) == FALSE)
  {
    Service->Log.AddLog("ini-file not found\n");
  }
  else
  {
    Service->Log.AddLog("ini-file succesfully loaded\n");
  }

  initdata = GetNewIni();
  SetDefaultInitdata(initdata);
}

void IniDriver::Release()
{
  // save the m_current_initdata data structure to the ini-file
  SaveToFilename(GetCurrentInitdata(), _defaultIniFilePath);

  FreeIni(m_default_ini);
  FreeIni(m_current_ini);
}
#include "fellow/os/null/IniDriver.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

ULO IniDriver::GetULOFromString(STR *value)
{
  return atoi(value);
}

int IniDriver::GetintFromString(STR *value)
{
  return atoi(value);
}

BOOLE IniDriver::GetBOOLEFromString(STR *value)
{
  return (atoi(value) == 1);
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

BOOLE IniDriver::SetOption(IniValues *initdata, STR *initoptionstr)
{
  STR *option, *value;
  BOOLE result;

  value = strchr(initoptionstr, '=');
  result = (value != NULL);
  if (result)
  {
    option = initoptionstr;
    *value++ = '\0';

    /* Standard initialization options */

    if (stricmp(option, "last_used_configuration") == 0)
    {
      if (strcmp(value, "") == 0)
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
    else if (stricmp(option, "last_used_cfg_dir") == 0)
    {
      initdata->SetLastUsedCfgDir(value);
    }
    else if (stricmp(option, "main_window_x_pos") == 0)
    {
      initdata->SetMainWindowXPos(GetintFromString(value));
    }
    else if (stricmp(option, "main_window_y_pos") == 0)
    {
      initdata->SetMainWindowYPos(GetintFromString(value));
    }
    else if (stricmp(option, "emu_window_x_pos") == 0)
    {
      initdata->SetEmulationWindowXPos(GetintFromString(value));
    }
    else if (stricmp(option, "emu_window_y_pos") == 0)
    {
      initdata->SetEmulationWindowYPos(GetintFromString(value));
    }
    else if (stricmp(option, "config_history_0") == 0)
    {
      initdata->SetConfigurationHistoryFilename(0, value);
    }
    else if (stricmp(option, "config_history_1") == 0)
    {
      initdata->SetConfigurationHistoryFilename(1, value);
    }
    else if (stricmp(option, "config_history_2") == 0)
    {
      initdata->SetConfigurationHistoryFilename(2, value);
    }
    else if (stricmp(option, "config_history_3") == 0)
    {
      initdata->SetConfigurationHistoryFilename(3, value);
    }
    else if (stricmp(option, "last_used_kick_image_dir") == 0)
    {
      initdata->SetLastUsedKickImageDir(value);
    }
    else if (stricmp(option, "last_used_key_dir") == 0)
    {
      initdata->SetLastUsedKeyDir(value);
    }
    else if (stricmp(option, "last_used_global_disk_dir") == 0)
    {
      initdata->SetLastUsedGlobalDiskDir(value);
    }
    else if (stricmp(option, "last_used_hdf_dir") == 0)
    {
      initdata->SetLastUsedHdfDir(value);
    }
    else if (stricmp(option, "last_used_mod_dir") == 0)
    {
      initdata->SetLastUsedModDir(value);
    }
    else if (stricmp(option, "last_used_cfg_tab") == 0)
    {
      initdata->SetLastUsedCfgTab(GetULOFromString(value));
    }
    else if (stricmp(option, "last_used_statefile_dir") == 0)
    {
      initdata->SetLastUsedStateFileDir(value);
    }
    else if (stricmp(option, "last_used_preset_rom_dir") == 0)
    {
      initdata->SetLastUsedPresetROMDir(value);
    }
    else if (stricmp(option, "pause_emulation_when_window_loses_focus") == 0)
    {
      initdata->SetPauseEmulationWhenWindowLosesFocus(stricmp(value, "true") == 0);
    }
    else if (stricmp(option, "gfx_debug_immediate_rendering") == 0)
    {
      initdata->SetGfxDebugImmediateRendering(stricmp(value, "true") == 0);
    }
    else
      result = FALSE;
  }
  else
    result = FALSE;
  return result;
}

BOOLE IniDriver::LoadIniFile(IniValues *initdata, FILE *inifile)
{
  char line[256];
  while (!feof(inifile))
  {
    fgets(line, 256, inifile);
    StripTrailingNewlines(line);
    initdata->SetOption(line);
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

BOOLE IniDriver::SaveToFilename(IniValues *initdata, STR *filename)
{
  FILE *inifile;
  BOOLE result;

  inifile = fopen(filename, "w");
  result = (inifile != NULL);
  if (result)
  {
    result = SaveToFile(initdata, inifile);
    fclose(inifile);
  }
  return result;
}

BOOLE IniDriver::SaveOptions(IniValues *initdata, FILE *inifile)
{
  fprintf(inifile, "ini_description=%s\n", initdata->GetDescription());
  fprintf(inifile, "main_window_x_pos=%d\n", initdata->GetMainWindowXPos());
  fprintf(inifile, "main_window_y_pos=%d\n", initdata->GetMainWindowYPos());
  fprintf(inifile, "emu_window_x_pos=%d\n", initdata->GetEmulationWindowXPos());
  fprintf(inifile, "emu_window_y_pos=%d\n", initdata->GetEmulationWindowYPos());
  fprintf(inifile, "config_history_0=%s\n", initdata->GetConfigurationHistoryFilename(0));
  fprintf(inifile, "config_history_1=%s\n", initdata->GetConfigurationHistoryFilename(1));
  fprintf(inifile, "config_history_2=%s\n", initdata->GetConfigurationHistoryFilename(2));
  fprintf(inifile, "config_history_3=%s\n", initdata->GetConfigurationHistoryFilename(3));
  fprintf(inifile, "last_used_configuration=%s\n", initdata->GetCurrentConfigurationFilename());
  fprintf(inifile, "last_used_cfg_dir=%s\n", initdata->GetLastUsedCfgDir());
  fprintf(inifile, "last_used_cfg_tab=%u\n", initdata->GetLastUsedCfgTab());
  fprintf(inifile, "last_used_kick_image_dir=%s\n", initdata->GetLastUsedKickImageDir());
  fprintf(inifile, "last_used_key_dir=%s\n", initdata->GetLastUsedKeyDir());
  fprintf(inifile, "last_used_global_disk_dir=%s\n", initdata->GetLastUsedGlobalDiskDir());
  fprintf(inifile, "last_used_hdf_dir=%s\n", initdata->GetLastUsedHdfDir());
  fprintf(inifile, "last_used_mod_dir=%s\n", initdata->GetLastUsedModDir());
  fprintf(inifile, "last_used_statefile_dir=%s\n", initdata->GetLastUsedStateFileDir());
  fprintf(inifile, "last_used_preset_rom_dir=%s\n", initdata->GetLastUsedPresetROMDir());
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

void IniDriver::Startup()
{
  IniValues *initdata = GetNewIni();
  SetCurrentInitdata(initdata);

  // TODO: The only OS specific in this file was the MAX_PATH macro, find some other way around that
  STR defaultFilepath[CFG_FILENAME_LENGTH];
  Service->Fileops.GetGenericFileName(defaultFilepath, "WinFellow", INI_FILENAME);

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

void IniDriver::Shutdown()
{
  // save the m_current_initdata data structure to the ini-file
  SaveToFilename(GetCurrentInitdata(), ini_filename);

  FreeIni(m_default_ini);
  FreeIni(m_current_ini);
}
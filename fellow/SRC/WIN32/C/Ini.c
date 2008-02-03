/*===========================================================================*/
/* WinFellow Amiga Emulator                                                  */
/* Ini file for Windows                                                      */
/* Author: Worfje (worfje@gmx.net)                                           */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

#include "defs.h"
#include <windows.h>
#include <direct.h>
#include "ini.h"
#include "draw.h"
#include "fellow.h"

/*============================================================================*/
/* The actual iniManager instance                                             */
/*============================================================================*/

iniManager ini_manager;

/*============================================================================*/
/* Remove unwanted newline chars on the end of a string                       */
/*============================================================================*/

static void iniStripTrailingNewlines(STR *line) {
  size_t length = strlen(line);
  while ((length > 0) && 
	 ((line[length - 1] == '\n') || (line[length - 1] == '\r')))
    line[--length] = '\0';
}

/*============================================================================*/
/* Read specific options from a string                                        */
/* These verify the options, or at least return a default value on error      */
/*============================================================================*/

static ULO iniGetULOFromString(STR *value) {
  return atoi(value);
}

static BOOLE iniGetBOOLEFromString(STR *value) {
	return (atoi(value) == 1);
}

/*============================================================================*/
/* struct ini property access functions                                       */
/*============================================================================*/

void iniSetDescription(ini *initdata, STR *description) {
  strcpy(initdata->m_description, description);
}

STR *iniGetDescription(ini *initdata) {
  return initdata->m_description;
}

void iniSetConfigurationHistoryFilename(ini *initdata, ULO index, STR *cfgfilename) {
  strncpy(initdata->m_configuration_history[index], cfgfilename, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(initdata->m_configuration_history[index]);
}

STR *iniGetConfigurationHistoryFilename(ini *initdata, ULO index) {
  return initdata->m_configuration_history[index];
}

void iniSetMainWindowXPos(ini *initdata, ULO xpos) {
  initdata->m_mainwindowxposition = xpos;
}

ULO iniGetMainWindowXPos(ini *initdata) {
  return initdata->m_mainwindowxposition;
}

void iniSetMainWindowYPos(ini *initdata, ULO ypos) {
  initdata->m_mainwindowyposition = ypos;
}

ULO iniGetMainWindowYPos(ini *initdata) {
  return initdata->m_mainwindowyposition;
}

void iniSetEmulationWindowXPos(ini *initdata, ULO xpos) {
  initdata->m_emulationwindowxposition = xpos;
}

ULO iniGetEmulationWindowXPos(ini *initdata) {
  return initdata->m_emulationwindowxposition;
}

void iniSetEmulationWindowYPos(ini *initdata, ULO ypos) {
  initdata->m_emulationwindowyposition = ypos;
}

ULO iniGetEmulationWindowYPos(ini *initdata) {
  return initdata->m_emulationwindowyposition;
}

void iniSetMainWindowPosition(ini *initdata, ULO mainwindowxpos, ULO mainwindowypos) {
  iniSetMainWindowXPos(initdata, mainwindowxpos);
  iniSetMainWindowYPos(initdata, mainwindowypos);
} 

void iniSetEmulationWindowPosition(ini *initdata, ULO emulationwindowxpos, ULO emulationwindowypos) {
  iniSetEmulationWindowXPos(initdata, emulationwindowxpos);
  iniSetEmulationWindowYPos(initdata, emulationwindowypos);
}

void iniSetCurrentConfigurationFilename(ini *initdata, STR *configuration) {
  strncpy(initdata->m_current_configuration, configuration, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(initdata->m_current_configuration);
}

STR *iniGetCurrentConfigurationFilename(ini *initdata) {
  return initdata->m_current_configuration;
}

void iniSetLastUsedCfgDir(ini *initdata, STR *directory) {
  strncpy(initdata->m_lastusedconfigurationdir, directory, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(initdata->m_lastusedconfigurationdir);
}

STR *iniGetLastUsedCfgDir(ini *initdata) {
  return initdata->m_lastusedconfigurationdir;
}

void iniSetLastUsedCfgTab(ini *initdata, ULO cfgTab) {
  initdata->m_lastusedconfigurationtab = cfgTab;
}

ULO iniGetLastUsedCfgTab(ini *initdata) {
  return initdata->m_lastusedconfigurationtab;
}

void iniSetLastUsedKickImageDir(ini *initdata, STR *directory) {
  strncpy(initdata->m_lastusedkickimagedir, directory, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(initdata->m_lastusedkickimagedir);
}

STR *iniGetLastUsedKickImageDir(ini *initdata) {
  return initdata->m_lastusedkickimagedir;
}

void iniSetLastUsedKeyDir(ini *initdata, STR *directory) {
  strncpy(initdata->m_lastusedkeydir, directory, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(initdata->m_lastusedkeydir);
}

STR *iniGetLastUsedKeyDir(ini *initdata) {
  return initdata->m_lastusedkeydir;
}

void iniSetLastUsedGlobalDiskDir(ini *initdata, STR *directory) {
  strncpy(initdata->m_lastusedglobaldiskdir, directory, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(initdata->m_lastusedkeydir);
}

STR *iniGetLastUsedGlobalDiskDir(ini *initdata) {
  return initdata->m_lastusedglobaldiskdir;
}

void iniSetLastUsedHdfDir(ini *initdata, STR *directory) {
  strncpy(initdata->m_lastusedhdfdir, directory, CFG_FILENAME_LENGTH);
  //fsNavigMakeRelativePath(initdata->m_lastusedkeydir);
}

STR *iniGetLastUsedHdfDir(ini *initdata) {
  return initdata->m_lastusedhdfdir;
}

void iniSetLastUsedModDir(ini *initdata, STR *directory) {
  strncpy(initdata->m_lastusedmoddir, directory, CFG_FILENAME_LENGTH);
}

STR *iniGetLastUsedModDir(ini *initdata) {
  return initdata->m_lastusedmoddir;
}

/*============================================================================*/
/* struct iniManager property access functions                                */
/*============================================================================*/

void iniManagerSetCurrentInitdata(iniManager *inimanager, ini *initdata) {
  inimanager->m_current_ini = initdata;
}

ini *iniManagerGetCurrentInitdata(iniManager *inimanager) {
  return inimanager->m_current_ini;
}

void iniManagerSetDefaultInitdata(iniManager *inimanager, ini *initdata) {
  inimanager->m_default_ini = initdata;
}

ini *iniManagerGetDefaultInitdata(iniManager *inimanager) {
  return inimanager->m_default_ini;
}

/*============================================================================*/
/* Sets all options to default values                                         */
/*============================================================================*/

void iniSetDefaults(ini *initdata) {
  ULO i;

  /*==========================================================================*/
  /* Default ini-file description                                             */
  /*==========================================================================*/

  iniSetDescription(initdata, FELLOWLONGVERSION);
  
  /*==========================================================================*/
  /* Default window positions                                                 */
  /*==========================================================================*/

  iniSetMainWindowXPos(initdata, 0);
  iniSetMainWindowYPos(initdata, 0);
  iniSetEmulationWindowXPos(initdata, 0);
  iniSetEmulationWindowYPos(initdata, 0);
  for (i=0; i<4; i++) {
    iniSetConfigurationHistoryFilename(initdata, i, "");
  }

  /*==========================================================================*/
  /* Default configuration filename                                           */
  /*==========================================================================*/ 

  iniSetCurrentConfigurationFilename(initdata, "./configurations/default.wfc");

  /*==========================================================================*/
  /* Default kickimage and key directories                                    */
  /*==========================================================================*/ 

  iniSetLastUsedCfgDir(initdata, "");
  iniSetLastUsedCfgTab(initdata, 0);
  iniSetLastUsedKickImageDir(initdata, "");
  iniSetLastUsedKeyDir(initdata, "");
  iniSetLastUsedGlobalDiskDir(initdata, "");  
  iniSetLastUsedHdfDir(initdata, "");
}

/*============================================================================*/
/* Load initialization file                                                   */
/*============================================================================*/

static BOOLE iniLoadIniFile(ini *initdata, FILE *inifile) {
  char line[256];
  while (!feof(inifile)) {
    fgets(line, 256, inifile);
    iniStripTrailingNewlines(line);
    iniSetOption(initdata, line);
  }
  return TRUE;
}

BOOLE iniLoadIniFromFilename(ini *inidata, STR *filename) {
  FILE *inifile;
  BOOLE result;
  
  inifile = fopen(filename, "r");
  result = (inifile != NULL);
  if (result) {
    result = iniLoadIniFile(inidata, inifile);
    fclose(inifile);
  }
  return result;
}

/*============================================================================*/
/* Save initdata to file                                                      */
/*============================================================================*/

static BOOLE iniSaveToFile(ini *initdata, FILE *inifile) {
  return iniSaveOptions(initdata, inifile);
}

BOOLE iniSaveToFilename(ini *initdata, STR *filename) {
  FILE *inifile;
  BOOLE result;

  inifile = fopen(filename, "w");
  result = (inifile != NULL);
  if (result) {
    result = iniSaveToFile(initdata, inifile);
    fclose(inifile);
  }
  return result;
}

/*============================================================================*/
/* Set initialization options                                                 */
/* Returns TRUE if the option was recognized                                  */
/*============================================================================*/

BOOLE iniSetOption(ini *initdata, STR *initoptionstr) {
  STR *option, *value;
  BOOLE result;
  struct stat bla; 

  value = strchr(initoptionstr, '=');
  result = (value != NULL);
  if (result) {
    option = initoptionstr;
    *value++ = '\0';

    /* Standard initialization options */

    if (stricmp(option, "last_used_configuration") == 0) {
		if (strcmp(value, "") == 0) {
			iniSetCurrentConfigurationFilename(initdata, "./configurations/default.wfc");
			_mkdir("./configurations");
		} else {
			if (stat(value,&bla) != 0) {
				iniSetCurrentConfigurationFilename(initdata, "./configurations/default.wfc");
				_mkdir("./configurations");
			} else {
				iniSetCurrentConfigurationFilename(initdata, value);
			}
		}
    }
	else if (stricmp(option, "last_used_cfg_dir") == 0) {
      iniSetLastUsedCfgDir(initdata, value);
	}
    else if (stricmp(option, "main_window_x_pos") == 0) {
      iniSetMainWindowXPos(initdata, iniGetULOFromString(value));
	}
	else if (stricmp(option, "main_window_y_pos") == 0) {
	  iniSetMainWindowYPos(initdata, iniGetULOFromString(value));
	}
	else if (stricmp(option, "emu_window_x_pos") == 0) {
	  iniSetEmulationWindowXPos(initdata, iniGetULOFromString(value));
	}
	else if (stricmp(option, "emu_window_y_pos") == 0) {
	  iniSetEmulationWindowYPos(initdata, iniGetULOFromString(value));
	}
	else if (stricmp(option, "config_history_0") == 0) {
	  iniSetConfigurationHistoryFilename(initdata, 0, value);
	}
	else if (stricmp(option, "config_history_1") == 0) {
	  iniSetConfigurationHistoryFilename(initdata, 1, value);
	}
	else if (stricmp(option, "config_history_2") == 0) {
	  iniSetConfigurationHistoryFilename(initdata, 2, value);
	}
	else if (stricmp(option, "config_history_3") == 0) {
	  iniSetConfigurationHistoryFilename(initdata, 3, value);
	}
	else if (stricmp(option, "last_used_kick_image_dir") == 0) {
      iniSetLastUsedKickImageDir(initdata, value);
    }
	else if (stricmp(option, "last_used_key_dir") == 0) {
      iniSetLastUsedKeyDir(initdata, value);
    }
	else if (stricmp(option, "last_used_global_disk_dir") == 0) {
      iniSetLastUsedGlobalDiskDir(initdata, value);
    }
	else if (stricmp(option, "last_used_hdf_dir") == 0) {
      iniSetLastUsedHdfDir(initdata, value);
    }
	else if (stricmp(option, "last_used_mod_dir") == 0) {
      iniSetLastUsedModDir(initdata, value);
    }
	else if (stricmp(option, "last_used_cfg_tab") == 0) {
      iniSetLastUsedCfgTab(initdata, iniGetULOFromString(value));
    }
    else result = FALSE;
  }
  else result = FALSE;
  return result;
}

BOOLE iniSaveOptions(ini *initdata, FILE *inifile) {
  fprintf(inifile, "ini_description=%s\n", iniGetDescription(initdata));
  fprintf(inifile, "main_window_x_pos=%d\n", iniGetMainWindowXPos(initdata));
  fprintf(inifile, "main_window_y_pos=%d\n", iniGetMainWindowYPos(initdata));
  fprintf(inifile, "emu_window_x_pos=%d\n", iniGetEmulationWindowXPos(initdata));
  fprintf(inifile, "emu_window_y_pos=%d\n", iniGetEmulationWindowYPos(initdata));
  fprintf(inifile, "config_history_0=%s\n", iniGetConfigurationHistoryFilename(initdata,0));
  fprintf(inifile, "config_history_1=%s\n", iniGetConfigurationHistoryFilename(initdata,1));
  fprintf(inifile, "config_history_2=%s\n", iniGetConfigurationHistoryFilename(initdata,2));
  fprintf(inifile, "config_history_3=%s\n", iniGetConfigurationHistoryFilename(initdata,3));
  fprintf(inifile, "last_used_configuration=%s\n", iniGetCurrentConfigurationFilename(initdata));
  fprintf(inifile, "last_used_cfg_dir=%s\n", iniGetLastUsedCfgDir(initdata));
  fprintf(inifile, "last_used_cfg_tab=%d\n", iniGetLastUsedCfgTab(initdata));
  fprintf(inifile, "last_used_kick_image_dir=%s\n", iniGetLastUsedKickImageDir(initdata));
  fprintf(inifile, "last_used_key_dir=%s\n", iniGetLastUsedKeyDir(initdata));
  fprintf(inifile, "last_used_global_disk_dir=%s\n", iniGetLastUsedGlobalDiskDir(initdata));
  fprintf(inifile, "last_used_hdf_dir=%s\n", iniGetLastUsedHdfDir(initdata));
  fprintf(inifile, "last_used_mod_dir=%s\n", iniGetLastUsedModDir(initdata));
  return TRUE;
}

ini *iniManagerGetNewIni(iniManager *initdatamanager) {
	
	ini *initdata = (ini *) malloc(sizeof(ini));
	iniSetDefaults(initdata);
	return initdata;
}

void iniManagerFreeIni(iniManager *initdatamanager, ini *initdata) {

	free(initdata);
}

void iniManagerStartup(iniManager *initdatamanager) {
  
  ini *initdata = iniManagerGetNewIni(initdatamanager);
  iniManagerSetCurrentInitdata(initdatamanager, initdata);
  
  // load the ini-file into the m_current_initdata data structure
  if (iniLoadIniFromFilename(initdata, "winfellow.ini") == FALSE) {
	fellowAddLog("ini-file not found\n");
  } else {
	fellowAddLog("ini-file succesfully loaded\n");
  }

  initdata = iniManagerGetNewIni(initdatamanager);
  iniManagerSetDefaultInitdata(initdatamanager, initdata);
}

void iniManagerShutdown(iniManager *initdatamanager) {
  
  // save the m_current_initdata data structure to the ini-file
  iniSaveToFilename(iniManagerGetCurrentInitdata(initdatamanager), "winfellow.ini");

  iniManagerFreeIni(initdatamanager, initdatamanager->m_default_ini);
  iniManagerFreeIni(initdatamanager, initdatamanager->m_current_ini);
}

void iniStartup(void) {
  iniManagerStartup(&ini_manager);
}

void iniShutdown(void) {
  iniManagerShutdown(&ini_manager);
}

void iniEmulationStart(void) {
}

void iniEmulationStop(void) {
}
#ifndef INI_H
#define INI_H


/*============================================================================*/
/* struct that holds initialization data                                      */
/*============================================================================*/

typedef struct {

  STR	m_description[256];
	
  /*==========================================================================*/
  /* Holds current used configuration filename                                */
  /*==========================================================================*/

  STR  m_current_configuration[CFG_FILENAME_LENGTH];

  /*==========================================================================*/
  /* Window positions (Main Window, Emulation Window)                         */
  /*==========================================================================*/

  ULO  m_mainwindowxposition;
  ULO  m_mainwindowyposition;
  ULO  m_emulationwindowxposition;
  ULO  m_emulationwindowyposition;

  /*==========================================================================*/
  /* History of used config files                                             */
  /*==========================================================================*/

  STR  m_configuration_history[4][CFG_FILENAME_LENGTH];
  
  /*==========================================================================*/
  /* Holds last used directories                                              */
  /*==========================================================================*/

  STR  m_lastusedkeydir[CFG_FILENAME_LENGTH];
  STR  m_lastusedkickimagedir[CFG_FILENAME_LENGTH];
  STR  m_lastusedconfigurationdir[CFG_FILENAME_LENGTH];
  
} ini;

/*============================================================================*/
/* struct ini property access functions                                       */
/*============================================================================*/

extern STR *iniGetCurrentConfigurationFilename(ini *initdata);
extern void iniSetCurrentConfigurationFilename(ini *initdata, STR *configuration);
extern void iniSetLastUsedCfgDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedCfgDir(ini *initdata);
extern void iniSetMainWindowPosition(ini *initdata, ULO mainwindowxpos, ULO mainwindowypos);
extern void iniSetEmulationWindowPosition(ini *initdata, ULO emulationwindowxpos, ULO emulationwindowypos);
extern STR *iniGetConfigurationHistoryFilename(ini *initdata, ULO position);
extern void iniSetConfigurationHistoryFilename(ini *initdata, ULO position, STR *configuration);
extern STR *iniGetConfigurationHistoryFilename(ini *initdata, ULO position);
extern void iniSetConfigurationHistoryFilename(ini *initdata, ULO position, STR *cfgfilename);
extern void iniSetLastUsedKickImageDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedKickImageDir(ini *initdata);
extern void iniSetLastUsedKeyDir(ini *initdata, STR *directory);
extern STR *iniGetLastUsedKeyDir(ini *initdata);

/*============================================================================*/
/* struct iniManager                                                          */
/*============================================================================*/

typedef struct {
  ini *m_current_ini;
  ini *m_default_ini;
} iniManager;

/*============================================================================*/
/* struct iniManager property access functions                                */
/*============================================================================*/

extern void iniManagerSetCurrentInitdata(iniManager *initdatamanager,
				       ini *currentinitdata);
extern ini *iniManagerGetCurrentInitdata(iniManager *initdatamanager);
extern void iniManagerSetDefaultInitdata(iniManager *inimanager, ini *initdata);
extern ini *iniManagerGetDefaultInitdata(iniManager *inimanager);


/*============================================================================*/
/* struct iniManager utility functions                                        */
/*============================================================================*/

extern BOOLE iniManagerInitdataActivate(iniManager *initdatamanager);
extern ini *iniManagerGetNewInitdata(iniManager *initdatamanager);
extern void iniManagerFreeInitdata(iniManager *initdatamanager, ini *initdata);
extern void iniManagerStartup(iniManager *initdatamanager);
extern void iniManagerShutdown(iniManager *initdatamanager);


/*============================================================================*/
/* The actual iniManager instance                                             */
/*============================================================================*/

extern iniManager ini_manager;

#endif  /* INI_H */

#ifndef CONFIG_H
#define CONFIG_H

/*============================================================================*/
/* struct that holds a complete hardfile configuration                        */
/*============================================================================*/

typedef struct {
  STR   filename[CFG_FILENAME_LENGTH];
  BOOLE readonly;
  ULO   bytespersector;
  ULO   sectorspertrack;
  ULO   surfaces;
  ULO   reservedblocks;
} cfg_hardfile;


/*============================================================================*/
/* struct that holds a complete filesystem configuration                      */
/*============================================================================*/

typedef struct {
  STR   volumename[64];
  STR   rootpath[CFG_FILENAME_LENGTH];
  BOOLE readonly;
} cfg_filesys;


/*============================================================================*/
/* struct that holds a complete Fellow configuration                          */
/*============================================================================*/

typedef struct {

  STR	m_description[256];
	
  /*==========================================================================*/
  /* Floppy disk configuration                                                */
  /*==========================================================================*/

  STR    m_diskimage[4][CFG_FILENAME_LENGTH];
  BOOLE  m_diskenabled[4];
  BOOLE  m_diskreadonly[4];
  BOOLE  m_diskfast;
  STR    m_lastuseddiskdir[CFG_FILENAME_LENGTH];	

  /*==========================================================================*/
  /* Memory configuration                                                     */
  /*==========================================================================*/

  ULO    m_chipsize;
  ULO    m_fastsize;
  ULO    m_bogosize;
  STR    m_kickimage[CFG_FILENAME_LENGTH];
  STR    m_key[CFG_FILENAME_LENGTH];
  BOOLE  m_useautoconfig;


  /*==========================================================================*/
  /* Screen configuration                                                     */
  /*==========================================================================*/

  ULO	m_screenwidth;
  ULO	m_screenheight;
  ULO   m_screencolorbits;
  ULO   m_screenrefresh;
  BOOLE m_screenwindowed;
  BOOLE m_screendrawleds;
  ULO   m_frameskipratio;
  ULO   m_horisontalscale;
  ULO   m_verticalscale;
  BOOLE m_scanlines;
  BOOLE m_deinterlace;
  BOOLE m_measurespeed;


  /*==========================================================================*/
  /* Sound configuration                                                      */
  /*==========================================================================*/

  sound_emulations m_soundemulation;
  sound_rates      m_soundrate;
  BOOLE            m_soundstereo;
  BOOLE            m_sound16bits;
  sound_filters    m_soundfilter;
  BOOLE            m_soundWAVdump;
  BOOLE            m_notification;
  ULO              m_bufferlength;


  /*==========================================================================*/
  /* CPU configuration                                                        */
  /*==========================================================================*/

  cpu_types m_CPUtype;
  ULO m_CPUspeed;


  /*==========================================================================*/
  /* Custom chipset configuration                                             */
  /*==========================================================================*/

  BOOLE m_blitterfast;
  BOOLE m_ECSblitter;


  /*==========================================================================*/
  /* Hardfile configuration                                                   */
  /*==========================================================================*/

  felist* m_hardfiles;


  /*==========================================================================*/
  /* Filesystem configuration                                                 */
  /*==========================================================================*/

  felist* m_filesystems;
  BOOLE m_automount_drives;

  /*==========================================================================*/
  /* Game port configuration                                                  */
  /*==========================================================================*/

  gameport_inputs m_gameport[2];


  /*==========================================================================*/
  /* GUI configuration                                                        */
  /*==========================================================================*/

  BOOLE m_useGUI;


} cfg;

/*============================================================================*/
/* struct cfg property access functions                                       */
/*============================================================================*/

/*============================================================================*/
/* Floppy disk configuration property access                                  */
/*============================================================================*/

extern void cfgSetDiskImage(cfg *config, ULO index, STR *diskimage);
extern STR *cfgGetDiskImage(cfg *config, ULO index);
extern void cfgSetDiskEnabled(cfg *config, ULO index, BOOLE enabled);
extern BOOLE cfgGetDiskEnabled(cfg *config, ULO index);
extern void cfgSetDiskReadOnly(cfg *config, ULO index, BOOLE readonly);
extern BOOLE cfgGetDiskReadOnly(cfg *config, ULO index);
extern void cfgSetDiskFast(cfg *config, BOOLE fast);
extern BOOLE cfgGetDiskFast(cfg *config);  
extern void cfgSetLastUsedDiskDir(cfg *config, STR *directory);
extern STR *cfgGetLastUsedDiskDir(cfg *config);

/*============================================================================*/
/* Memory configuration property access                                       */
/*============================================================================*/

extern void cfgSetChipSize(cfg *config, ULO chipsize);
extern ULO cfgGetChipSize(cfg *config);
extern void cfgSetFastSize(cfg *config, ULO fastsize);
extern ULO cfgGetFastSize(cfg *config);
extern void cfgSetBogoSize(cfg *config, ULO bogosize);
extern ULO cfgGetBogoSize(cfg *config);
extern void cfgSetKickImage(cfg *config, STR *kickimage);
extern STR *cfgGetKickImage(cfg *config);
extern void cfgSetKey(cfg *config, STR *key);
extern STR *cfgGetKey(cfg *config);
extern void cfgSetUseAutoconfig(cfg *config, BOOLE useautoconfig);
extern BOOLE cfgGetUseAutoconfig(cfg *config);
extern void cfgSetAddress32Bit(cfg *config, BOOLE address32bit);
extern BOOLE cfgGetAddress32Bit(cfg *config);


/*===========================================================================*/
/* Host screen configuration property access                                 */
/*===========================================================================*/

extern void cfgSetScreenWidth(cfg *config, ULO screenwidth);
extern ULO cfgGetScreenWidth(cfg *config);
extern void cfgSetScreenHeight(cfg *config, ULO screenheight);
extern ULO cfgGetScreenHeight(cfg *config);
extern void cfgSetScreenColorBits(cfg *config, ULO screencolorbits);
extern ULO cfgGetScreenColorBits(cfg *config);
extern void cfgSetScreenRefresh(cfg *config, ULO screenrefresh);
extern ULO cfgGetScreenRefresh(cfg *config);
extern void cfgSetScreenWindowed(cfg *config, BOOLE screenwindowed);
extern BOOLE cfgGetScreenWindowed(cfg *config);
extern void cfgSetScreenDrawLEDs(cfg *config, BOOLE drawleds);
extern BOOLE cfgGetScreenDrawLEDs(cfg *config);


/*===========================================================================*/
/* Graphics emulation configuration property access                          */
/*===========================================================================*/

extern void cfgSetFrameSkipRatio(cfg *config, ULO frameskipratio);
extern ULO cfgGetFrameSkipRatio(cfg *config);
extern void cfgSetHorisontalScale(cfg *config, ULO horisontalscale);
extern ULO cfgGetHorisontalScale(cfg *config);
extern void cfgSetVerticalScale(cfg *config, ULO verticalscale);
extern ULO cfgGetVerticalScale(cfg *config);
extern void cfgSetScanlines(cfg *config, BOOLE scanlines);
extern BOOLE cfgGetScanlines(cfg *config);
extern void cfgSetDeinterlace(cfg *config, BOOLE deinterlace);
extern BOOLE cfgGetDeinterlace(cfg *config);


/*============================================================================*/
/* Sound configuration property access                                        */
/*============================================================================*/

extern void cfgSetSoundEmulation(cfg *config, sound_emulations soundemulation);
extern sound_emulations cfgGetSoundEmulation(cfg *config);
extern void cfgSetSoundRate(cfg *config, sound_rates soundrate);
extern sound_rates cfgGetSoundRate(cfg *config);
extern void cfgSetSoundStereo(cfg *config, BOOLE soundstereo);
extern BOOLE cfgGetSoundStereo(cfg *config);
extern void cfgSetSound16Bits(cfg *config, BOOLE soundbit);
extern BOOLE cfgGetSound16Bits(cfg *config);
extern void cfgSetSoundFilter(cfg *config, sound_filters soundfilter);
extern sound_filters cfgGetSoundFilter(cfg *config);
extern void cfgSetSoundWAVDump(cfg *config, BOOLE soundWAVdump);
extern BOOLE cfgGetSoundWAVDump(cfg *config);
extern void cfgSetSoundNotification(cfg *config, BOOLE notification);
extern BOOLE cfgGetSoundNotification(cfg *config);
extern void cfgSetSoundBufferLength(cfg *config, ULO buffer_length);
extern ULO cfgGetSoundBufferLength(cfg *config);


/*============================================================================*/
/* CPU configuration property access                                          */
/*============================================================================*/

extern void cfgSetCPUType(cfg *config, cpu_types CPUtype);
extern cpu_types cfgGetCPUType(cfg *config);
extern void cfgSetCPUSpeed(cfg *config, ULO CPUspeed);
extern ULO cfgGetCPUSpeed(cfg *config);


/*============================================================================*/
/* Custom chipset configuration property access                               */
/*============================================================================*/

extern void cfgSetBlitterFast(cfg *config, BOOLE blitterfast);
extern BOOLE cfgGetBlitterFast(cfg *config);
extern void cfgSetECSBlitter(cfg *config, BOOLE ECSblitter);
extern BOOLE cfgGetECSBlitter(cfg *config);


/*============================================================================*/
/* Hardfile configuration property access                                     */
/*============================================================================*/

extern ULO cfgGetHardfileCount(cfg *config);
extern cfg_hardfile cfgGetHardfile(cfg *config, ULO index);
extern void cfgHardfileAdd(cfg *config, cfg_hardfile *hardfile);
extern void cfgHardfileRemove(cfg *config, ULO index);
extern void cfgHardfilesRemove(cfg *config);
extern void cfgHardfilesFree(cfg *config);
extern void cfgSetHardfileUnitDefaults(cfg_hardfile *hardfile);
extern void cfgHardfileChange(cfg *config, cfg_hardfile *hardfile, ULO index);


/*============================================================================*/
/* Filesystem configuration property access                                   */
/*============================================================================*/

extern ULO cfgGetFilesystemCount(cfg *config);
extern cfg_filesys cfgGetFilesystem(cfg *config, ULO index);
extern void cfgFilesystemAdd(cfg *config, cfg_filesys *filesystem);
extern void cfgFilesystemRemove(cfg *config, ULO index);
extern void cfgFilesystemsRemove(cfg *config);
extern void cfgFilesystemsFree(cfg *config);
extern void cfgSetFilesystemUnitDefaults(cfg_filesys *filesystem);
extern void cfgFilesystemChange(cfg *config,
				cfg_filesys *filesystem, 
				ULO index);
extern void cfgSetFilesystemAutomountDrives(cfg *config,
					    BOOLE automount_drives);
extern BOOLE cfgGetFilesystemAutomountDrives(cfg *config);


/*============================================================================*/
/* Game port configuration property access                                    */
/*============================================================================*/

extern void cfgSetGameport(cfg *config, ULO index, gameport_inputs gameport);
extern gameport_inputs cfgGetGameport(cfg *config, ULO index);


/*============================================================================*/
/* GUI configuration property access                                          */
/*============================================================================*/

extern void cfgSetUseGUI(cfg *config, BOOLE useGUI);
extern BOOLE cfgGetUseGUI(cfg *config);


/*============================================================================*/
/* Various configuration property access                                      */
/*============================================================================*/

extern void cfgSetMeasureSpeed(cfg *config, BOOLE measurespeed);
extern BOOLE cfgGetMeasureSpeed(cfg *config);

/*============================================================================*/
/* cfg Utility Functions                                                      */
/*============================================================================*/

extern void cfgSetDefaults(cfg *config);
extern BOOLE cfgSetOption(cfg *config, STR *optionstr);
extern BOOLE cfgSaveOptions(cfg *config, FILE *cfgfile);
extern BOOLE cfgLoadFromFilename(cfg *config, STR *filename);
extern BOOLE cfgSaveToFilename(cfg *config, STR *filename);
extern void cfgSynopsis(cfg *config);

/*============================================================================*/
/* struct cfgManager                                                          */
/*============================================================================*/

typedef struct {
  cfg *m_currentconfig;
  cfg *m_original_config;
} cfgManager;


/*============================================================================*/
/* struct cfgManager property access functions                                */
/*============================================================================*/

extern void cfgManagerSetCurrentConfig(cfgManager *configmanager,
				       cfg *currentconfig);
extern cfg *cfgManagerGetCurrentConfig(cfgManager *configmanager);
extern void cfgManagerSetDefaultConfig(cfgManager *configmanager);


/*============================================================================*/
/* struct cfgManager utility functions                                        */
/*============================================================================*/

extern BOOLE cfgManagerConfigActivate(cfgManager *configmanager);
extern cfg *cfgManagerGetNewConfig(cfgManager *configmanager);
extern void cfgManagerFreeConfig(cfgManager *configmanager, cfg *config);
extern void cfgManagerStartup(cfgManager *configmanager,
			       int argc, 
			       char *argv[]);
extern void cfgManagerShutdown(cfgManager *configmanager);


/*============================================================================*/
/* The actual cfgManager instance                                             */
/*============================================================================*/

extern cfgManager cfg_manager;


#endif  /* CONFIG_H */


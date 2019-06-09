#ifndef CONFIG_H
#define CONFIG_H

#include "sound.h"
#include "CpuIntegration.h"
#include "gameport.h"
#include "listtree.h"
#include "fellow/api/module/IHardfileHandler.h"

using namespace fellow::api::module;

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
  rdb_status rdbstatus;
} cfg_hardfile;

/*============================================================================*/
/* struct that holds a complete filesystem configuration                      */
/*============================================================================*/

typedef struct {
  STR   volumename[64];
  STR   rootpath[CFG_FILENAME_LENGTH];
  BOOLE readonly;
} cfg_filesys;

typedef enum {
  DISPLAYSCALE_AUTO = 0,
  DISPLAYSCALE_1X = 1,
  DISPLAYSCALE_2X = 2,
  DISPLAYSCALE_3X = 3,
  DISPLAYSCALE_4X = 4
} DISPLAYSCALE;

typedef enum {
  DISPLAYSCALE_STRATEGY_SOLID = 0,
  DISPLAYSCALE_STRATEGY_SCANLINES = 1
} DISPLAYSCALE_STRATEGY;

typedef enum {
  DISPLAYDRIVER_DIRECTDRAW = 0,
  DISPLAYDRIVER_DIRECT3D11 = 1
} DISPLAYDRIVER;

typedef enum {
  GRAPHICSEMULATIONMODE_LINEEXACT  = 0,
  GRAPHICSEMULATIONMODE_CYCLEEXACT = 1
} GRAPHICSEMULATIONMODE;

/*============================================================================*/
/* struct that holds a complete Fellow configuration                          */
/* the struct is copied in cfgManagerGetCopyOfCurrentConfig()                 */
/* any new pointers to allocated structures must be taken into account there  */
/* when changed here.                                                         */
/*============================================================================*/

#define CONFIG_CURRENT_FILE_VERSION 2

typedef struct {

  ULO   m_configfileversion;
  STR	m_description[CFG_FILENAME_LENGTH];


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
  STR    m_kickimage_ext[CFG_FILENAME_LENGTH];
  STR    m_kickdescription[CFG_FILENAME_LENGTH];
  ULO    m_kickcrc32;
  STR    m_key[CFG_FILENAME_LENGTH];
  bool   m_useautoconfig;
  bool   m_rtc;


  /*==========================================================================*/
  /* Screen configuration                                                     */
  /*==========================================================================*/

  ULO	m_screenwidth;
  ULO	m_screenheight;
  ULO   m_screencolorbits;
  ULO   m_screenrefresh;
  bool  m_screenwindowed;

  bool  m_screendrawleds;
  ULO   m_frameskipratio;

  ULO m_clipleft;
  ULO m_cliptop;
  ULO m_clipright;
  ULO m_clipbottom;

  DISPLAYSCALE  m_displayscale;
  DISPLAYSCALE_STRATEGY m_displayscalestrategy;
  bool  m_deinterlace;
  bool  m_measurespeed;
  DISPLAYDRIVER m_displaydriver;
  GRAPHICSEMULATIONMODE m_graphicsemulationmode;


  /*==========================================================================*/
  /* Hold the option for using multiple graphical buffers                     */
  /*==========================================================================*/

  BOOLE  m_use_multiple_graphical_buffers;


  /*==========================================================================*/
  /* Sound configuration                                                      */
  /*==========================================================================*/

  sound_emulations     m_soundemulation;
  sound_rates          m_soundrate;
  bool                 m_soundstereo;
  bool                 m_sound16bits;
  sound_filters        m_soundfilter;
  ULO                  m_soundvolume;
  BOOLE                m_soundWAVdump;
  sound_notifications  m_notification;
  ULO                  m_bufferlength;


  /*==========================================================================*/
  /* CPU configuration                                                        */
  /*==========================================================================*/

  cpu_integration_models m_CPUtype;
  ULO m_CPUspeed;


  /*==========================================================================*/
  /* Custom chipset configuration                                             */
  /*==========================================================================*/

  BOOLE m_blitterfast;
  bool m_ECS;


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

  bool m_useGUI;


  /*==========================================================================*/
  /* tracking flags                                                           */
  /*==========================================================================*/

  BOOLE m_config_applied_once;
  BOOLE m_config_changed_since_save;

} cfg;

/*============================================================================*/
/* struct cfg property access functions                                       */
/*============================================================================*/

/*============================================================================*/
/* Configuration                                                              */
/*============================================================================*/

extern STR *cfgGetDescription(cfg *config);

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
extern void cfgSetKickImageExtended(cfg *config, STR *kickimageext);
extern STR *cfgGetKickImageExtended(cfg *config);
extern void cfgSetKickDescription(cfg *config, STR *kickdescription);
extern STR *cfgGetKickDescription(cfg *config);
extern void cfgSetKickCRC32(cfg *config, ULO kickcrc32);
extern ULO  cfgGetKickCRC32(cfg *config);
extern void cfgSetKey(cfg *config, STR *key);
extern STR *cfgGetKey(cfg *config);
extern void cfgSetUseAutoconfig(cfg *config, bool useautoconfig);
extern bool cfgGetUseAutoconfig(cfg *config);
extern BOOLE cfgGetAddress32Bit(cfg *config);
extern void cfgSetRtc(cfg *config, bool rtc);
extern bool cfgGetRtc(cfg *config);

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
extern void cfgSetScreenWindowed(cfg *config, bool screenwindowed);
extern bool cfgGetScreenWindowed(cfg *config);

extern void cfgSetScreenDrawLEDs(cfg *config, bool drawleds);
extern bool cfgGetScreenDrawLEDs(cfg *config);
extern void cfgSetUseMultipleGraphicalBuffers(cfg *config, BOOLE use_multiple_graphical_buffers);
extern BOOLE cfgGetUseMultipleGraphicalBuffers(cfg *config);
extern void cfgSetDisplayDriver(cfg *config, DISPLAYDRIVER display_driver);
extern DISPLAYDRIVER cfgGetDisplayDriver(cfg *config);

/*===========================================================================*/
/* Graphics emulation configuration property access                          */
/*===========================================================================*/

extern void  cfgSetFrameskipRatio (cfg *config, ULO frameskipratio);
extern ULO   cfgGetFrameskipRatio (cfg *config);

extern void cfgSetClipLeft(cfg *config, ULO left);
extern ULO cfgGetClipLeft(cfg *config);
extern void cfgSetClipTop(cfg *config, ULO top);
extern ULO cfgGetClipTop(cfg *config);
extern void cfgSetClipRight(cfg *config, ULO right);
extern ULO cfgGetClipRight(cfg *config);
extern void cfgSetClipBottom(cfg *config, ULO bottom);
extern ULO cfgGetClipBottom(cfg *config);

extern void  cfgSetDisplayScale(cfg *config, DISPLAYSCALE displayscale);
extern DISPLAYSCALE cfgGetDisplayScale(cfg *config);
extern void cfgSetDisplayScaleStrategy(cfg *config, DISPLAYSCALE_STRATEGY displayscalestrategy);
extern DISPLAYSCALE_STRATEGY cfgGetDisplayScaleStrategy(cfg *config);
extern void cfgSetDeinterlace(cfg *config, bool deinterlace);
extern bool cfgGetDeinterlace(cfg *config);
extern void cfgSetGraphicsEmulationMode(cfg *config, GRAPHICSEMULATIONMODE graphcisemulationmode);
extern GRAPHICSEMULATIONMODE cfgGetGraphicsEmulationMode(cfg *config);

/*============================================================================*/
/* Sound configuration property access                                        */
/*============================================================================*/

extern void cfgSetSoundEmulation(cfg *config, sound_emulations soundemulation);
extern sound_emulations cfgGetSoundEmulation(cfg *config);
extern void cfgSetSoundRate(cfg *config, sound_rates soundrate);
extern sound_rates cfgGetSoundRate(cfg *config);
extern void cfgSetSoundStereo(cfg *config, bool soundstereo);
extern bool cfgGetSoundStereo(cfg *config);
extern void cfgSetSound16Bits(cfg *config, bool soundbit);
extern bool cfgGetSound16Bits(cfg *config);
extern void cfgSetSoundFilter(cfg *config, sound_filters soundfilter);
extern sound_filters cfgGetSoundFilter(cfg *config);
extern void cfgSetSoundVolume(cfg *config, const ULO soundvolume);
extern ULO cfgGetSoundVolume(cfg *config);
extern void cfgSetSoundWAVDump(cfg *config, BOOLE soundWAVdump);
extern BOOLE cfgGetSoundWAVDump(cfg *config);
extern void cfgSetSoundNotification(cfg *config, sound_notifications soundnotification);
extern sound_notifications cfgGetSoundNotification(cfg *config);
extern void cfgSetSoundBufferLength(cfg *config, ULO buffer_length);
extern ULO cfgGetSoundBufferLength(cfg *config);


/*============================================================================*/
/* CPU configuration property access                                          */
/*============================================================================*/

extern void cfgSetCPUType(cfg *config, cpu_integration_models CPUtype);
extern cpu_integration_models cfgGetCPUType(cfg *config);
extern void cfgSetCPUSpeed(cfg *config, ULO CPUspeed);
extern ULO cfgGetCPUSpeed(cfg *config);


/*============================================================================*/
/* Custom chipset configuration property access                               */
/*============================================================================*/

extern void cfgSetBlitterFast(cfg *config, BOOLE blitterfast);
extern BOOLE cfgGetBlitterFast(cfg *config);
extern void cfgSetECS(cfg *config, bool ecs);
extern bool cfgGetECS(cfg *config);


/*============================================================================*/
/* Hardfile configuration property access                                     */
/*============================================================================*/

extern ULO cfgGetHardfileCount(cfg *config);
extern cfg_hardfile cfgGetHardfile(cfg *config, ULO index);
extern void cfgHardfileAdd(cfg *config, cfg_hardfile *hardfile);
extern void cfgHardfileRemove(cfg *config, ULO index);
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

extern void cfgSetUseGUI(cfg *config, bool useGUI);
extern bool cfgGetUseGUI(cfg *config);


/*============================================================================*/
/* Various configuration property access                                      */
/*============================================================================*/

extern void cfgSetMeasureSpeed(cfg *config, bool measurespeed);
extern bool cfgGetMeasureSpeed(cfg *config);

/*============================================================================*/
/* cfg Utility Functions                                                      */
/*============================================================================*/

extern void cfgSetDefaults(cfg *config);
extern BOOLE cfgSetOption(cfg *config, STR *optionstr);
extern BOOLE cfgSaveOptions(cfg *config, FILE *cfgfile);
extern bool cfgLoadFromFilename(cfg *, const STR *, const bool);
extern BOOLE cfgSaveToFilename(cfg *config, STR *filename);
extern void cfgSynopsis(cfg *config);

/*============================================================================*/
/* struct cfgManager                                                          */
/*============================================================================*/

typedef struct {
  cfg *m_currentconfig;
} cfgManager;


/*============================================================================*/
/* struct cfgManager property access functions                                */
/*============================================================================*/

extern void cfgManagerSetCurrentConfig(cfgManager *configmanager, cfg *currentconfig);
extern cfg *cfgManagerGetCurrentConfig(cfgManager *configmanager);
extern cfg *cfgManagerGetCopyOfCurrentConfig(cfgManager *configmanager);


/*============================================================================*/
/* struct cfgManager utility functions                                        */
/*============================================================================*/

extern BOOLE cfgManagerConfigurationActivate(cfgManager *configmanager);
extern cfg *cfgManagerGetNewConfig(cfgManager *configmanager);
extern void cfgManagerFreeConfig(cfgManager *configmanager, cfg *config);
extern void cfgManagerStartup(cfgManager *configmanager, int argc, char *argv[]);
extern void cfgManagerShutdown(cfgManager *configmanager);


/*============================================================================*/
/* The actual cfgManager instance                                             */
/*============================================================================*/

extern cfgManager cfg_manager;

extern void cfgStartup(int argc, char **argv);
extern void cfgShutdown(void);

/*============================================================================*/
/* tracking flag functions                                                    */
/*============================================================================*/

extern void  cfgSetConfigAppliedOnce(cfg *, BOOLE);
extern BOOLE cfgGetConfigAppliedOnce(cfg *);
extern void  cfgSetConfigChangedSinceLastSave(cfg *, BOOLE);
extern BOOLE cfgGetConfigChangedSinceLastSave(cfg *);


#endif  /* CONFIG_H */


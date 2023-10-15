#pragma once

#include "CustomChipset/Sound/SoundConfiguration.h"
#include "CpuIntegration.h"
#include "gameport.h"
#include "ListTree.h"
#include "fellow/api/module/IHardfileHandler.h"

#include <string>

/*============================================================================*/
/* struct that holds a complete hardfile configuration                        */
/*============================================================================*/

typedef struct {
  char   filename[CFG_FILENAME_LENGTH];
  BOOLE readonly;
  uint32_t   bytespersector;
  uint32_t   sectorspertrack;
  uint32_t   surfaces;
  uint32_t   reservedblocks;
  fellow::api::module::rdb_status rdbstatus;
} cfg_hardfile;

/*============================================================================*/
/* struct that holds a complete filesystem configuration                      */
/*============================================================================*/

typedef struct {
  char   volumename[64];
  char   rootpath[CFG_FILENAME_LENGTH];
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
  GRAPHICSEMULATIONMODE_LINEEXACT = 0,
  GRAPHICSEMULATIONMODE_CYCLEEXACT = 1
} GRAPHICSEMULATIONMODE;

/*============================================================================*/
/* struct that holds a complete Fellow configuration                          */
/* the struct is copied in cfgManagerGetCopyOfCurrentConfig()                 */
/* any new pointers to allocated structures must be taken into account there  */
/* when changed here.                                                         */
/*============================================================================*/

#define CONFIG_CURRENT_FILE_VERSION 3

typedef struct {

  uint32_t   m_configfileversion;
  char	m_description[CFG_FILENAME_LENGTH];


  /*==========================================================================*/
  /* Floppy disk configuration                                                */
  /*==========================================================================*/

  char    m_diskimage[4][CFG_FILENAME_LENGTH];
  BOOLE  m_diskenabled[4];
  BOOLE  m_diskreadonly[4];
  BOOLE  m_diskfast;
  char    m_lastuseddiskdir[CFG_FILENAME_LENGTH];


  /*==========================================================================*/
  /* Memory configuration                                                     */
  /*==========================================================================*/

  uint32_t    m_chipsize;
  uint32_t    m_fastsize;
  uint32_t    m_bogosize;
  char    m_kickimage[CFG_FILENAME_LENGTH];
  char    m_kickimage_ext[CFG_FILENAME_LENGTH];
  char    m_kickdescription[CFG_FILENAME_LENGTH];
  uint32_t    m_kickcrc32;
  char    m_key[CFG_FILENAME_LENGTH];
  bool   m_useautoconfig;
  bool   m_rtc;


  /*==========================================================================*/
  /* Screen configuration                                                     */
  /*==========================================================================*/

  uint32_t	m_screenwidth;
  uint32_t	m_screenheight;
  uint32_t   m_screencolorbits;
  uint32_t   m_screenrefresh;
  bool  m_screenwindowed;

  bool  m_screendrawleds;
  uint32_t   m_frameskipratio;

  uint32_t m_clipleft;
  uint32_t m_cliptop;
  uint32_t m_clipright;
  uint32_t m_clipbottom;

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

  CustomChipset::sound_emulations     m_soundemulation;
  CustomChipset::sound_rates          m_soundrate;
  bool                 m_soundstereo;
  bool                 m_sound16bits;
  CustomChipset::sound_filters        m_soundfilter;
  uint32_t                  m_soundvolume;
  BOOLE                m_soundWAVdump;
  CustomChipset::sound_notifications  m_notification;
  uint32_t                  m_bufferlength;


  /*==========================================================================*/
  /* CPU configuration                                                        */
  /*==========================================================================*/

  cpu_integration_models m_CPUtype;
  uint32_t m_CPUspeed;


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
  char m_filesystem_device_name_prefix[CFG_FILENAME_LENGTH];

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

extern char* cfgGetDescription(cfg* config);

/*============================================================================*/
/* Floppy disk configuration property access                                  */
/*============================================================================*/

extern void cfgSetDiskImage(cfg* config, uint32_t index, const std::string& diskimage);
extern const char* cfgGetDiskImage(cfg* config, uint32_t index);
extern void cfgSetDiskEnabled(cfg* config, uint32_t index, BOOLE enabled);
extern BOOLE cfgGetDiskEnabled(cfg* config, uint32_t index);
extern void cfgSetDiskReadOnly(cfg* config, uint32_t index, BOOLE readonly);
extern BOOLE cfgGetDiskReadOnly(cfg* config, uint32_t index);
extern void cfgSetDiskFast(cfg* config, BOOLE fast);
extern BOOLE cfgGetDiskFast(cfg* config);
extern void cfgSetLastUsedDiskDir(cfg* config, const std::string& directory);
extern const char* cfgGetLastUsedDiskDir(cfg* config);

/*============================================================================*/
/* Memory configuration property access                                       */
/*============================================================================*/

extern void cfgSetChipSize(cfg* config, uint32_t chipsize);
extern uint32_t cfgGetChipSize(cfg* config);
extern void cfgSetFastSize(cfg* config, uint32_t fastsize);
extern uint32_t cfgGetFastSize(cfg* config);
extern void cfgSetBogoSize(cfg* config, uint32_t bogosize);
extern uint32_t cfgGetBogoSize(cfg* config);
extern void cfgSetKickImage(cfg* config, const std::string& kickimage);
extern const char* cfgGetKickImage(cfg* config);
extern void cfgSetKickImageExtended(cfg* config, const std::string& kickimageext);
extern const char* cfgGetKickImageExtended(cfg* config);
extern void cfgSetKickDescription(cfg* config, const std::string& kickdescription);
extern const char* cfgGetKickDescription(cfg* config);
extern void cfgSetKickCRC32(cfg* config, uint32_t kickcrc32);
extern uint32_t  cfgGetKickCRC32(cfg* config);
extern void cfgSetKey(cfg* config, const std::string& key);
extern const char* cfgGetKey(cfg* config);
extern void cfgSetUseAutoconfig(cfg* config, bool useautoconfig);
extern bool cfgGetUseAutoconfig(cfg* config);
extern BOOLE cfgGetAddress32Bit(cfg* config);
extern void cfgSetRtc(cfg* config, bool rtc);
extern bool cfgGetRtc(cfg* config);

/*===========================================================================*/
/* Host screen configuration property access                                 */
/*===========================================================================*/

extern void cfgSetScreenWidth(cfg* config, uint32_t screenwidth);
extern uint32_t cfgGetScreenWidth(cfg* config);
extern void cfgSetScreenHeight(cfg* config, uint32_t screenheight);
extern uint32_t cfgGetScreenHeight(cfg* config);
extern void cfgSetScreenColorBits(cfg* config, uint32_t screencolorbits);
extern uint32_t cfgGetScreenColorBits(cfg* config);
extern void cfgSetScreenRefresh(cfg* config, uint32_t screenrefresh);
extern uint32_t cfgGetScreenRefresh(cfg* config);
extern void cfgSetScreenWindowed(cfg* config, bool screenwindowed);
extern bool cfgGetScreenWindowed(cfg* config);

extern void cfgSetScreenDrawLEDs(cfg* config, bool drawleds);
extern bool cfgGetScreenDrawLEDs(cfg* config);
extern void cfgSetUseMultipleGraphicalBuffers(cfg* config, BOOLE use_multiple_graphical_buffers);
extern BOOLE cfgGetUseMultipleGraphicalBuffers(cfg* config);
extern void cfgSetDisplayDriver(cfg* config, DISPLAYDRIVER display_driver);
extern DISPLAYDRIVER cfgGetDisplayDriver(cfg* config);

/*===========================================================================*/
/* Graphics emulation configuration property access                          */
/*===========================================================================*/

extern void  cfgSetFrameskipRatio(cfg* config, uint32_t frameskipratio);
extern uint32_t   cfgGetFrameskipRatio(cfg* config);

extern void cfgSetClipLeft(cfg* config, uint32_t left);
extern uint32_t cfgGetClipLeft(cfg* config);
extern void cfgSetClipTop(cfg* config, uint32_t top);
extern uint32_t cfgGetClipTop(cfg* config);
extern void cfgSetClipRight(cfg* config, uint32_t right);
extern uint32_t cfgGetClipRight(cfg* config);
extern void cfgSetClipBottom(cfg* config, uint32_t bottom);
extern uint32_t cfgGetClipBottom(cfg* config);

extern void  cfgSetDisplayScale(cfg* config, DISPLAYSCALE displayscale);
extern DISPLAYSCALE cfgGetDisplayScale(cfg* config);
extern void cfgSetDisplayScaleStrategy(cfg* config, DISPLAYSCALE_STRATEGY displayscalestrategy);
extern DISPLAYSCALE_STRATEGY cfgGetDisplayScaleStrategy(cfg* config);
extern void cfgSetDeinterlace(cfg* config, bool deinterlace);
extern bool cfgGetDeinterlace(cfg* config);
extern void cfgSetGraphicsEmulationMode(cfg* config, GRAPHICSEMULATIONMODE graphcisemulationmode);
extern GRAPHICSEMULATIONMODE cfgGetGraphicsEmulationMode(cfg* config);

/*============================================================================*/
/* Sound configuration property access                                        */
/*============================================================================*/

extern void cfgSetSoundEmulation(cfg* config, CustomChipset::sound_emulations soundemulation);
extern CustomChipset::sound_emulations cfgGetSoundEmulation(cfg* config);
extern void cfgSetSoundRate(cfg* config, CustomChipset::sound_rates soundrate);
extern CustomChipset::sound_rates cfgGetSoundRate(cfg* config);
extern void cfgSetSoundStereo(cfg* config, bool soundstereo);
extern bool cfgGetSoundStereo(cfg* config);
extern void cfgSetSound16Bits(cfg* config, bool soundbit);
extern bool cfgGetSound16Bits(cfg* config);
extern void cfgSetSoundFilter(cfg* config, CustomChipset::sound_filters soundfilter);
extern CustomChipset::sound_filters cfgGetSoundFilter(cfg* config);
extern void cfgSetSoundVolume(cfg* config, const uint32_t soundvolume);
extern uint32_t cfgGetSoundVolume(cfg* config);
extern void cfgSetSoundWAVDump(cfg* config, BOOLE soundWAVdump);
extern BOOLE cfgGetSoundWAVDump(cfg* config);
extern void cfgSetSoundNotification(cfg* config, CustomChipset::sound_notifications soundnotification);
extern CustomChipset::sound_notifications cfgGetSoundNotification(cfg* config);
extern void cfgSetSoundBufferLength(cfg* config, uint32_t buffer_length);
extern uint32_t cfgGetSoundBufferLength(cfg* config);


/*============================================================================*/
/* CPU configuration property access                                          */
/*============================================================================*/

extern void cfgSetCPUType(cfg* config, cpu_integration_models CPUtype);
extern cpu_integration_models cfgGetCPUType(cfg* config);
extern void cfgSetCPUSpeed(cfg* config, uint32_t CPUspeed);
extern uint32_t cfgGetCPUSpeed(cfg* config);


/*============================================================================*/
/* Custom chipset configuration property access                               */
/*============================================================================*/

extern void cfgSetBlitterFast(cfg* config, BOOLE blitterfast);
extern BOOLE cfgGetBlitterFast(cfg* config);
extern void cfgSetECS(cfg* config, bool ecs);
extern bool cfgGetECS(cfg* config);


/*============================================================================*/
/* Hardfile configuration property access                                     */
/*============================================================================*/

extern uint32_t cfgGetHardfileCount(cfg* config);
extern cfg_hardfile cfgGetHardfile(cfg* config, uint32_t index);
extern void cfgHardfileAdd(cfg* config, cfg_hardfile* hardfile);
extern void cfgHardfileRemove(cfg* config, uint32_t index);
extern void cfgHardfilesFree(cfg* config);
extern void cfgSetHardfileUnitDefaults(cfg_hardfile* hardfile);
extern void cfgHardfileChange(cfg* config, cfg_hardfile* hardfile, uint32_t index);


/*============================================================================*/
/* Filesystem configuration property access                                   */
/*============================================================================*/

extern uint32_t cfgGetFilesystemCount(cfg* config);
extern cfg_filesys cfgGetFilesystem(cfg* config, uint32_t index);
extern void cfgFilesystemAdd(cfg* config, cfg_filesys* filesystem);
extern void cfgFilesystemRemove(cfg* config, uint32_t index);
extern void cfgFilesystemsFree(cfg* config);
extern void cfgSetFilesystemUnitDefaults(cfg_filesys* filesystem);
extern void cfgFilesystemChange(cfg* config,
  cfg_filesys* filesystem,
  uint32_t index);
extern void cfgSetFilesystemAutomountDrives(cfg* config,
  BOOLE automount_drives);
extern BOOLE cfgGetFilesystemAutomountDrives(cfg* config);

extern void cfgSetFilesystemDeviceNamePrefix(cfg* config, const std::string& prefix);
extern const char* cfgGetFilesystemDeviceNamePrefix(cfg* config);


/*============================================================================*/
/* Game port configuration property access                                    */
/*============================================================================*/

extern void cfgSetGameport(cfg* config, uint32_t index, gameport_inputs gameport);
extern gameport_inputs cfgGetGameport(cfg* config, uint32_t index);


/*============================================================================*/
/* GUI configuration property access                                          */
/*============================================================================*/

extern void cfgSetUseGUI(cfg* config, bool useGUI);
extern bool cfgGetUseGUI(cfg* config);


/*============================================================================*/
/* Various configuration property access                                      */
/*============================================================================*/

extern void cfgSetMeasureSpeed(cfg* config, bool measurespeed);
extern bool cfgGetMeasureSpeed(cfg* config);

/*============================================================================*/
/* cfg Utility Functions                                                      */
/*============================================================================*/

extern void cfgSetDefaults(cfg* config);
extern BOOLE cfgSetOption(cfg* config, const char* optionstr);
extern BOOLE cfgSaveOptions(cfg* config, FILE* cfgfile);
extern bool cfgLoadFromFilename(cfg*, const char*, const bool);
extern BOOLE cfgSaveToFilename(cfg* config, const char* filename);
extern void cfgSynopsis(cfg* config);

/*============================================================================*/
/* struct cfgManager                                                          */
/*============================================================================*/

typedef struct {
  cfg* m_currentconfig;
} cfgManager;


/*============================================================================*/
/* struct cfgManager property access functions                                */
/*============================================================================*/

extern void cfgManagerSetCurrentConfig(cfgManager* configmanager, cfg* currentconfig);
extern cfg* cfgManagerGetCurrentConfig(cfgManager* configmanager);
extern cfg* cfgManagerGetCopyOfCurrentConfig(cfgManager* configmanager);


/*============================================================================*/
/* struct cfgManager utility functions                                        */
/*============================================================================*/

extern BOOLE cfgManagerConfigurationActivate(cfgManager* configmanager);
extern cfg* cfgManagerGetNewConfig(cfgManager* configmanager);
extern void cfgManagerFreeConfig(cfgManager* configmanager, cfg* config);
extern void cfgManagerStartup(cfgManager* configmanager, int argc, char* argv[]);
extern void cfgManagerShutdown(cfgManager* configmanager);


/*============================================================================*/
/* The actual cfgManager instance                                             */
/*============================================================================*/

extern cfgManager cfg_manager;

extern void cfgStartup(int argc, const char** argv);
extern void cfgShutdown();

/*============================================================================*/
/* tracking flag functions                                                    */
/*============================================================================*/

extern void  cfgSetConfigAppliedOnce(cfg*, BOOLE);
extern BOOLE cfgGetConfigAppliedOnce(cfg*);
extern void  cfgSetConfigChangedSinceLastSave(cfg*, BOOLE);
extern BOOLE cfgGetConfigChangedSinceLastSave(cfg*);

#pragma once

#include <list>
#include "fellow/api/Services.h"
#include "fellow/application/HostRenderer.h"

enum class SelectFileFlags
{
  FSEL_ROM = 0,
  FSEL_ADF = 1,
  FSEL_KEY = 2,
  FSEL_HDF = 3,
  FSEL_WFC = 4,
  FSEL_MOD = 5,
  FSEL_FST = 6
};

struct wgui_drawmode
{
  int id;
  ULO width;
  ULO height;
  ULO refresh;
  ULO colorbits;
  STR name[32];

  bool operator<(const wgui_drawmode &dm)
  {
    if (width < dm.width)
    {
      return true;
    }
    else if (width == dm.width)
    {
      return height < dm.height;
    }
    return false;
  }

  wgui_drawmode(const DisplayMode *dm)
  {
    id = -1;
    height = dm->Height;
    refresh = dm->Refresh;
    width = dm->Width;
    colorbits = dm->Bits;
    name[0] = '\0';
  }
};

typedef std::list<wgui_drawmode> wgui_drawmode_list;

struct wgui_drawmodes
{
  ULO numberof16bit;
  ULO numberof24bit;
  ULO numberof32bit;
  LON comboxbox16bitindex;
  LON comboxbox24bitindex;
  LON comboxbox32bitindex;
  wgui_drawmode_list res16bit;
  wgui_drawmode_list res24bit;
  wgui_drawmode_list res32bit;
};

struct wgui_preset
{
  STR strPresetFilename[CFG_FILENAME_LENGTH];
  STR strPresetDescription[CFG_FILENAME_LENGTH];
};

/*===========================================================================*/
/* This is the generic interface that must be implemented to create a GUI    */
/* Fellow                                                                    */
/*===========================================================================*/

extern STR *wguiExtractPath(STR *);

extern void wguiStartup();
extern void wguiStartupPost();
extern void wguiShutdown();
extern bool wguiCheckEmulationNecessities();
extern BOOLE wguiEnter();
extern void wguiRequester(fellow::api::FELLOW_REQUESTER_TYPE type, STR *szMessage);
extern void wguiInsertCfgIntoHistory(STR *cfgfilenametoinsert);
extern void wguiSetProcessDPIAwareness(const char *pszAwareness);

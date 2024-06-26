#pragma once

#include <list>
#include "FellowMain.h"
#include "Renderer.h"

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
  uint32_t id;
  uint32_t width;
  uint32_t height;
  uint32_t refresh;
  uint32_t colorbits;
  char name[32];

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

  wgui_drawmode(draw_mode *dm)
  {
    height = dm->height;
    refresh = dm->refresh;
    width = dm->width;
    colorbits = dm->bits;
  }
};

typedef std::list<wgui_drawmode> wgui_drawmode_list;

struct wgui_drawmodes
{
  uint32_t numberof16bit;
  uint32_t numberof24bit;
  uint32_t numberof32bit;
  int32_t comboxbox16bitindex;
  int32_t comboxbox24bitindex;
  int32_t comboxbox32bitindex;
  wgui_drawmode_list res16bit;
  wgui_drawmode_list res24bit;
  wgui_drawmode_list res32bit;

  bool HasFullscreenModes() const
  {
    return !res16bit.empty() || !res24bit.empty() || !res32bit.empty();
  }
};

struct wgui_preset
{
  char strPresetFilename[CFG_FILENAME_LENGTH];
  char strPresetDescription[CFG_FILENAME_LENGTH];
};

/*===========================================================================*/
/* This is the generic interface that must be implemented to create a GUI    */
/* Fellow                                                                    */
/*===========================================================================*/

extern BOOLE wguiSaveFile(HWND hwndDlg, const char *filename, uint32_t filenamesize, const char *title, SelectFileFlags selectFileType);
extern char *wguiExtractPath(char *);

extern void wguiStartup();
extern void wguiStartupPost();
extern void wguiShutdown();
extern BOOLE wguiCheckEmulationNecessities();
extern BOOLE wguiEnter();
extern void wguiRequester(const char *szMessage, UINT uType);
extern void wguiShowRequester(const char *szMessage, FELLOW_REQUESTER_TYPE requesterType);
extern void wguiInsertCfgIntoHistory(const char *cfgfilenametoinsert);
extern void wguiSetProcessDPIAwareness(const char *pszAwareness);

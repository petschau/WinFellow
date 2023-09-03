#ifndef WGUI_H
#define WGUI_H

#include <list>
#include "DRAW.H"

typedef enum {
  FSEL_ROM = 0,
  FSEL_ADF = 1,
  FSEL_KEY = 2,
  FSEL_HDF = 3,
  FSEL_WFC = 4,
  FSEL_MOD = 5,
  FSEL_FST = 6
} SelectFileFlags;

struct wgui_drawmode
{
  ULO id;
  ULO width;
  ULO height;
  ULO refresh;
  ULO colorbits;
  STR name[32];

  bool operator<(const wgui_drawmode& dm)
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

typedef struct {
  ULO numberof16bit;
  ULO numberof24bit;
  ULO numberof32bit;
  LON comboxbox16bitindex;
  LON comboxbox24bitindex;
  LON comboxbox32bitindex;
  wgui_drawmode_list res16bit;
  wgui_drawmode_list res24bit;
  wgui_drawmode_list res32bit;
} wgui_drawmodes;

typedef struct {
  STR strPresetFilename[CFG_FILENAME_LENGTH];
  STR strPresetDescription[CFG_FILENAME_LENGTH];
} wgui_preset;

/*===========================================================================*/
/* This is the generic interface that must be implemented to create a GUI    */
/* Fellow                                                                    */
/*===========================================================================*/

extern BOOLE wguiSaveFile(HWND, STR *, ULO, STR *, SelectFileFlags);
extern STR *wguiExtractPath(STR *);

extern void wguiStartup(void);
extern void wguiStartupPost(void);
extern void wguiShutdown(void);
extern BOOLE wguiCheckEmulationNecessities(void);
extern BOOLE wguiEnter(void);
extern void wguiRequester(STR *szMessage, UINT uType);
extern void wguiInsertCfgIntoHistory(STR *cfgfilenametoinsert);
extern void wguiSetProcessDPIAwareness(const char *pszAwareness);

#endif /* End of WGUI_H */


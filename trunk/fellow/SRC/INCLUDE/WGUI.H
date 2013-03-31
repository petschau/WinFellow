#ifndef WGUI_H
#define WGUI_H

// 
#include "listtree.h"

typedef enum {
  FSEL_ROM = 0,
  FSEL_ADF = 1,
  FSEL_KEY = 2,
  FSEL_HDF = 3,
  FSEL_WFC = 4,
  FSEL_MOD = 5,
  FSEL_FST = 6
} SelectFileFlags;

typedef struct {
  ULO id;
  ULO width;
  ULO height;
  ULO refresh;
	BOOLE windowed;
	ULO colorbits;
  STR name[32];
} wgui_drawmode;

typedef struct {
	ULO numberofwindowed;
	ULO numberof16bit;
	ULO numberof24bit;
	ULO numberof32bit;
	LON comboxbox16bitindex;
	LON comboxbox24bitindex;
	LON comboxbox32bitindex;
	felist *reswindowed;
	felist *res16bit;
	felist *res24bit;
	felist *res32bit;
} wgui_drawmodes;

/*===========================================================================*/
/* This is the generic interface that must be implemented to create a GUI    */
/* Fellow                                                                    */
/*===========================================================================*/

#ifdef WGUI

extern BOOLE wguiSaveFile(HWND, STR *, ULO, STR *, SelectFileFlags);
extern STR *wguiExtractPath(STR *);

extern void wguiStartup(void);
extern void wguiStartupPost(void);
extern void wguiShutdown(void);
extern BOOLE wguiCheckEmulationNecessities(void);
extern BOOLE wguiEnter(void);
extern void wguiRequester(STR *line1, STR *line2, STR *line3);
extern void wguiInsertCfgIntoHistory(STR *cfgfilenametoinsert);

#else

/* null implementation */

static void wguiPreStartup(void) {}
static void wguiStartup(void) {}
static void wguiShutdown(void) {}
static void wguiEnter(void) {}
static void wguiInsertCfgIntoHistory(STR *cfgfilenametoinsert) {}
static void wguiRequester(STR *line1, STR *line2, STR *line3) {}

#endif /* End of WGUI */

#endif /* End of WGUI_H */


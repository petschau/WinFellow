#pragma once

#include <list>

#include "fellow/api/defs.h"
#include "fellow/application/DisplayMode.h"

struct wgui_drawmode
{
  int id;
  ULO width;
  ULO height;
  ULO refresh;
  ULO colorbits;
  STR name[32];

  bool operator<(const wgui_drawmode &dm) const
  {
    if (width < dm.width)
    {
      return true;
    }

    if (width == dm.width)
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

#pragma once

#include "fellow/application/WGui.h"

// GUI functions that only exist in win32 used by other win32 support files, not the core
extern BOOLE wguiSaveFile(HWND, STR *, ULO, const char *, SelectFileFlags); // Used by modrip_win32

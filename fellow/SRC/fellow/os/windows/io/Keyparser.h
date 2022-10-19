#pragma once

#include "fellow/api/defs.h"
#include "fellow/chipset/Keycodes.h"

BOOLE prsReadFile(char *szFilename, UBY *pc_to_am, kbd_drv_pc_symbol key_repl[2][8]);
BOOLE prsWriteFile(char *szFilename, UBY *pc_to_am, kbd_drv_pc_symbol key_repl[2][8]);

#pragma once

BOOLE prsReadFile( char *szFilename, uint8_t *pc_to_am, kbd_drv_pc_symbol key_repl[2][8] );
BOOLE prsWriteFile( char *szFilename, uint8_t *pc_to_am, kbd_drv_pc_symbol key_repl[2][8] );

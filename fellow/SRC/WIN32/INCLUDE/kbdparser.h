#ifndef KBDPARSER_H
#define KBDPARSER_H

BOOLE prsReadFile( char *szFilename, UBY *pc_to_am, kbd_drv_pc_symbol key_repl[2][8] );
BOOLE prsWriteFile( char *szFilename, UBY *pc_to_am, kbd_drv_pc_symbol key_repl[2][8] );

#endif // KBDPARSER_H
/*===========================================================================*/
/* Fellow Amiga Emulator                                                     */
/* Keyboard mapping file parser                                              */
/* Author: Marco Nova (novamarco@hotmail.com)                                */
/*                                                                           */
/* This file is under the GNU Public License (GPL)                           */
/*===========================================================================*/

/* ---------------- KNOWN BUGS/FIXLIST ----------------- 
- translation from keynames to DX keys should be perfectioned
- conflicts in the keys choosen by the user
*/

/* ---------------- CHANGE LOG ----------------- 
Friday, January 05, 2001: nova
- fixed problem if the last line is not an empty line
- fixed handling of joystick key replacement
- added prsWriteFile function to save the current state of the key mappings
- now you can specify only the keys you want to change

Tuesday, September 19, 2000: nova
- added autofire support

Tuesday, September 05, 2000: nova
- added parsing of replacement keys for the gameport
- added a third parameter to prsReadFile for the array of joy replacement ** internal **
*/


#include "defs.h"
#include "keycodes.h"
#include "kbdparser.h"
#include <stdio.h>


extern UBY kbd_drv_pc_symbol_to_amiga_scancode[106];

/*===========================================================================*/
/* windows key names to be used in the mapfile								 */
/*===========================================================================*/

#define MAX_PC_NAMES	106

extern STR* kbd_drv_pc_symbol_to_string[MAX_PC_NAMES];
#define pc_keys kbd_drv_pc_symbol_to_string

extern int symbol_to_DIK_kbddrv[PCK_LAST_KEY];


/*===========================================================================*/
/* amiga key names to be used in the mapfile								 */
/*===========================================================================*/

#define MAX_AMIGA_NAMES		96

STR *amiga_keys[MAX_AMIGA_NAMES] = {
	"ESCAPE",
	"GRAVE",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"0",
	"MINUS",
	"EQUALS",
	"BACKSLASH2",
	"BACKSPACE",
	"TAB",
	"Q",
	"W",
	"E",
	"R",
	"T",
	"Y",
	"U",
	"I",
	"O",
	"P",
	"LEFT_BRACKET",
	"RIGHT_BRACKET",
	"RETURN",
	"CTRL",
	"CAPS_LOCK",
	"A",
	"S",
	"D",
	"F",
	"G",
	"H",
	"J",
	"K",
	"L",
	"SEMICOLON",
	"APOSTROPHE",
	"BACKSLASH",
	"LEFT_SHIFT",
	"LESS_THAN",
	"Z",
	"X",
	"C",
	"V",
	"B",
	"N",
	"M",
	"COMMA",
	"PERIOD",
	"SLASH",
	"RIGHT_SHIFT",
	"LEFT_ALT",
	"LEFT_AMIGA",
	"SPACE",
	"RIGHT_AMIGA",
	"RIGHT_ALT",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"NUMPAD_LEFT_BRACKET",
	"NUMPAD_RIGHT_BRACKET",
	"NUMPAD_DIVIDE",
	"NUMPAD_MULTIPLY",
	"NUMPAD_7",
	"NUMPAD_8",
	"NUMPAD_9",
	"NUMPAD_MINUS",
	"NUMPAD_4",
	"NUMPAD_5",
	"NUMPAD_6",
	"NUMPAD_PLUS",
	"NUMPAD_1",
	"NUMPAD_2",
	"NUMPAD_3",
	"NUMPAD_ENTER",
	"NUMPAD_0",
	"NUMPAD_DOT",
	"HELP",
	"DELETE",
	"UP",
	"LEFT",
	"DOWN",
	"RIGHT"
};

/*===========================================================================*/
/* map for the amiga key names												 */
/*===========================================================================*/

UBY amiga_scancode[MAX_AMIGA_NAMES] = {
	A_ESCAPE,
    A_GRAVE,
	A_1,
	A_2,
	A_3,
	A_4,
	A_5,
	A_6,
	A_7,
	A_8,
	A_9,
	A_0,
	A_MINUS,
	A_EQUALS,
	A_BACKSLASH2,
	A_BACKSPACE,
	A_TAB,
	A_Q,
	A_W,
	A_E,
	A_R,
	A_T,
	A_Y,
	A_U,
	A_I,
	A_O,
	A_P,
	A_LEFT_BRACKET,
	A_RIGHT_BRACKET,
	A_RETURN,
	A_CTRL,
	A_CAPS_LOCK,
	A_A,
	A_S,
	A_D,
	A_F,
	A_G,
	A_H,
	A_J,
	A_K,
	A_L,
	A_SEMICOLON,
	A_APOSTROPHE,
	A_BACKSLASH,
	A_LEFT_SHIFT,
	A_LESS_THAN,
	A_Z,
	A_X,
	A_C,
	A_V,
	A_B,
	A_N,
	A_M,
	A_COMMA,
	A_PERIOD,
	A_SLASH,
	A_LEFT_SHIFT,
	A_LEFT_ALT,
	A_LEFT_AMIGA,
	A_SPACE,
	A_RIGHT_AMIGA,
	A_RIGHT_ALT,
	A_F1,
	A_F2,
	A_F3,
	A_F4,
	A_F5,
	A_F6,
	A_F7,
	A_F8,
	A_F9,
	A_F10,
	A_NUMPAD_LEFT_BRACKET,
	A_NUMPAD_RIGHT_BRACKET,
	A_NUMPAD_DIVIDE,
	A_NUMPAD_MULTIPLY,
	A_NUMPAD_7,
	A_NUMPAD_8,
	A_NUMPAD_9,
	A_NUMPAD_MINUS,
	A_NUMPAD_4,
	A_NUMPAD_5,
	A_NUMPAD_6,
	A_NUMPAD_PLUS,
	A_NUMPAD_1,
	A_NUMPAD_2,
	A_NUMPAD_3,
	A_NUMPAD_ENTER,
	A_NUMPAD_0,
	A_NUMPAD_DOT,
	A_HELP,
	A_DELETE,
	A_UP,
	A_LEFT,
	A_DOWN,
	A_RIGHT
};


#define MAX_KEY_REPLACEMENT		16
#define FIRST_KEY2_REPLACEMENT	8		// MAX_KEY_REPLACEMENT / 2


// the order of the replacement_keys array must coincide with the enum kbd_drv_joykey_directions in kbddrv.c

STR *replacement_keys[MAX_KEY_REPLACEMENT] = {
	"JOYKEY1_LEFT",
	"JOYKEY1_RIGHT",
	"JOYKEY1_UP",
	"JOYKEY1_DOWN",
	"JOYKEY1_FIRE0",
	"JOYKEY1_FIRE1",
	"JOYKEY1_AUTOFIRE0",
	"JOYKEY1_AUTOFIRE1",
	"JOYKEY2_LEFT",
	"JOYKEY2_RIGHT",
	"JOYKEY2_UP",
	"JOYKEY2_DOWN",
	"JOYKEY2_FIRE0",
	"JOYKEY2_FIRE1",
	"JOYKEY2_AUTOFIRE0",
	"JOYKEY2_AUTOFIRE1"
};


/*===========================================================================*/
/* Get index of the key														 */
/*===========================================================================*/

int prsGetKeyIndex( STR *szKey, STR *Keys[], int MaxKeys )
{
	int i = 0;
	for( i = 0; i < MaxKeys; i++ )
	{
		if( stricmp( szKey, Keys[i] ) == 0 )
			return i;
	}
	return -1;
}

/*===========================================================================*/
/* trim the left and right spaces and tab chars								 */
/*===========================================================================*/

STR *prsTrim( STR *line )
{
	int i = 0, j = strlen( line ) - 1;
	STR *p = line;

	// trim starting space and tab
	while(( i < j ) && ((line[i] == '\t') || (line[i] == ' '))) i++;
	if( i > j )
		return line;

	if( i )
		p = &line[i];

	// trim ending space and tab and new line
	while( (j > i) && (
		(line[j] == ' ') || 
		(line[j] == '\t') ||
		(line[j] == '\r') ||
		(line[j] == '\n')
		)) j--;
	if( j >= i )
		line[j+1] = '\0';

	return p;
}

/*===========================================================================*/
/* read the line and get Amiga key name and Pc key name						 */
/*===========================================================================*/

BOOLE prsGetAmigaName( STR *line, STR **pAm, STR **pWin )
{
	int i = 0, j = 0;
	*pAm = line;

	while( line[i] && (line[i] != '=')) i++;
	if( !line[i] )
		return TRUE;

	line[i] = '\0';
	*pAm = prsTrim( line );

	*pWin = prsTrim( &line[i+1] );

	return !pAm || !pWin;
}

/*===========================================================================*/
/* read the mapping file and set the map array								 */
/*===========================================================================*/

BOOLE prsReadFile( char *szFilename, UBY *pc_to_am, kbd_drv_pc_symbol key_repl[2][8] )
{
	FILE *f = NULL;
	char line[256], *pAmigaName = NULL, *pWinName = NULL;
	int AmigaIndex, PcIndex, ReplIndex;

	f = fopen( szFilename, "r" );
	if( !f ) {
    fellowAddLog( "cannot open filename %s: %s\n", szFilename, strerror( errno ));
		return TRUE;
	}

	// clear array

	for( PcIndex = 0; PcIndex < MAX_PC_NAMES; PcIndex++ )
		pc_to_am[PcIndex] = A_NONE;

	while( !feof( f ))
	{
		if( !fgets( line, 256, f ))
			break;

		// test if it's a comment line
		if( line[0] == ';' )
			continue;

		if( prsGetAmigaName( line, &pAmigaName, &pWinName ))
			continue;

		ReplIndex = -1;
		AmigaIndex = prsGetKeyIndex( pAmigaName, amiga_keys, MAX_AMIGA_NAMES );
		PcIndex = prsGetKeyIndex( pWinName, pc_keys, MAX_PC_NAMES );

		if( AmigaIndex < 0 )
			ReplIndex = prsGetKeyIndex( pAmigaName, replacement_keys, MAX_KEY_REPLACEMENT );

		if(( AmigaIndex < 0 ) && ( ReplIndex < 0 )) {
			fellowAddLog( "Amiga key: %s unrecognized\n", pAmigaName );
			continue;
		}

		if( PcIndex < 0 ) {
			fellowAddLog( "Pc    key: %s unrecognized\n", pWinName );
			continue;
		}

		if( AmigaIndex >= 0 )
			pc_to_am[PcIndex] = amiga_scancode[AmigaIndex];
		else
		{
			if( ReplIndex < FIRST_KEY2_REPLACEMENT )
				key_repl[0][ReplIndex] = symbol_to_DIK_kbddrv[ PcIndex ];
			else
				key_repl[1][ReplIndex - FIRST_KEY2_REPLACEMENT] = symbol_to_DIK_kbddrv[ PcIndex ];
		}
	}
	fclose( f );

	return FALSE;
}

BOOLE prsWriteFile( char *szFilename, UBY *pc_to_am, kbd_drv_pc_symbol key_repl[2][8] )
{
	FILE *f = NULL;
	char line[256], *pAmigaName = NULL, *pWinName = NULL;
	int AmigaIndex, PcIndex, ReplIndex;

	f = fopen( szFilename, "w" );
	if( !f )
	{
		fellowAddLog( "cannot open filename %s: %s\n", szFilename, strerror( errno ));
		return TRUE;
	}

#ifdef _DEBUG
  fellowAddLog( "rewriting mapping file %s\n", szFilename );
#endif

  for( AmigaIndex = 0; AmigaIndex < MAX_AMIGA_NAMES; AmigaIndex++ ) {
    line[0] = '\0';
    for( PcIndex = 0; PcIndex < MAX_PC_NAMES; PcIndex++ ) {
      if( pc_to_am[PcIndex] == amiga_scancode[ AmigaIndex ] ) {
        if( line[0] )
          fputs( line, f );
        sprintf( line, "%s = %s\n", amiga_keys[ AmigaIndex ], pc_keys[PcIndex] );
      }
    }
    if( line[0] )
      fputs( line, f );
    else {
      sprintf( line, ";%s = NONE\n", amiga_keys[ AmigaIndex ] );
      fputs( line, f );
    }
  }
  for( AmigaIndex = 0; AmigaIndex < MAX_KEY_REPLACEMENT; AmigaIndex++ ) {
    if( AmigaIndex < FIRST_KEY2_REPLACEMENT )
      sprintf( line, "%s = %s\n", replacement_keys[ AmigaIndex ], pc_keys[ key_repl[0][AmigaIndex] ] );
    else
      sprintf( line, "%s = %s\n", replacement_keys[ AmigaIndex ], pc_keys[ key_repl[1][AmigaIndex-FIRST_KEY2_REPLACEMENT] ] );

    fputs( line, f );
  }

	fclose( f );

	return FALSE;
}


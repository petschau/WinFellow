/*============================================================================*/
/* Fellow Amiga Emulator - zlib wrapper                                       */
/*                                                                            */
/* Author: Torsten Enderling (carfesh@gmx.net)                                */
/*         (Wraps zlib code to have one simple call for decompression.)       */
/*                                                                            */
/* originates from minigzip.c                                                 */
/*                                                                            */
/* This file is under the GNU General Public License (GPL)                    */
/*============================================================================*/
/*============================================================================*/
/* Changelog:                                                                 */
/* ==========                                                                 */
/* 02/14/2001: added compression of files                                     */
/* 10/03/2000: first version.                                                 */
/*============================================================================*/

#include "defs.h"
#include "zlib.h"

/*========================================================*/
/* Decompress the file named src into the file named dest */
/* return TRUE if succesful, FALSE on failure             */
/*========================================================*/

BOOLE gzUnpack(const char *src, const char *dest)
{
    gzFile	input;
    FILE	*output;
    char	buffer[1<<14];
    int		length;

    if((output = fopen(dest, "wb")) == NULL) return FALSE;
    if((input  = gzopen(src, "rb")) == NULL) return FALSE;

	for(;;)
	{
        length = gzread(input, buffer, sizeof(buffer));
        if(length < 0) return FALSE;
        if(length == 0) break;
        if((int)fwrite(buffer, 1, (unsigned)length, output) != length) return FALSE;
    }

    if(fclose(output)) return FALSE;
    if(gzclose(input) != Z_OK) return FALSE;
	return TRUE;
}

/*======================================================*/
/* Compress the file named src into the file named dest */
/* with standard compression and ratio 9                */
/* return TRUE if succesful, FALSE on failure           */
/*======================================================*/

BOOLE gzPack(const char *src, const char *dest)
{
	FILE *input;
	gzFile output;
	char outmode[20];
	char buffer[1<<14];
    int length;
    int error;

    strcpy(outmode, "wb9 ");

	if((input  = fopen(src, "rb"))      == NULL) return FALSE;
    if((output = gzopen(dest, outmode)) == NULL) return FALSE;;

    for(;;) 
	{
        length = fread(buffer, 1, sizeof(buffer), input);
        if(ferror(input)) return FALSE;
        if(length == 0) break;
        if(gzwrite(output, buffer, (unsigned)length) != length) return FALSE;
    }
	
	if(fclose(input)) return FALSE;
	if(gzclose(output) != Z_OK) return FALSE;

	return TRUE;
}

/*=========================================================================*/
/* Fellow Amiga Emulator - zlib wrapper                                    */
/*                                                                         */
/* @(#) $Id: zlibwrap.c,v 1.2.2.1 2004-05-27 10:05:27 carfesh Exp $        */
/*                                                                         */
/* Author: Torsten Enderling (carfesh@gmx.net)                             */
/*         (Wraps zlib code to have one simple call for decompression.)    */
/*                                                                         */
/* originates from minigzip.c                                              */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

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

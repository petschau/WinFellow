/*=========================================================================*/
/* Fellow - xDMS wrapper                                                   */
/*                                                                         */
/* Author: Torsten Enderling                                               */
/*         (Wraps xDMS code to have one simple call for decompression.)    */
/*                                                                         */
/* originates from xdms.c                                                  */
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

#include <stdio.h>

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#ifdef _FELLOW_DEBUG_CRT_MALLOC
#include <crtdbg.h>
#endif

#include <string.h>
#include <ctype.h>

#include "cdata.h"
#include "pfile.h"
#include "crc_csum.h"

USHORT dmsUnpack(char *src, char *dest)
{
	return Process_File(src, dest, CMD_UNPACK, 0, 0, 0);
}

void dmsErrMsg(USHORT err, char *i, char *o, char *errMess)
{
    switch (err) {
        case NO_PROBLEM:
        case FILE_END:
            return;
        case ERR_NOMEMORY:
            sprintf(errMess,"Not enough memory for buffers !\n");
            break;
        case ERR_CANTOPENIN:
            sprintf(errMess,"Can't open %s for reading\n",i);
            break;
        case ERR_CANTOPENOUT:
            sprintf(errMess,"Can't open %s for writing !\n",o);
            break;
        case ERR_NOTDMS:
            sprintf(errMess,"File %s is not a DMS archive !\n",i);
            break;
        case ERR_SREAD:
            sprintf(errMess,"Error reading file %s : unexpected end of file !\n",i);
            break;
        case ERR_HCRC:
            sprintf(errMess,"Error in file %s : header CRC error !\n",i);
            break;
        case ERR_NOTTRACK:
            sprintf(errMess,"Error in file %s : track header not found !\n",i);
            break;
        case ERR_BIGTRACK:
            sprintf(errMess,"Error in DMS file: track too big");
            break;
        case ERR_THCRC:
            sprintf(errMess,"Error in file %s : track header CRC error !\n",i);
            break;
        case ERR_TDCRC:
            sprintf(errMess,"Error in file %s : track data CRC error !\n",i);
            break;
        case ERR_CSUM:
            sprintf(errMess,"Error in file %s : checksum error after unpacking !\n",i);
            break;
        case ERR_CANTWRITE:
            sprintf(errMess,"Error : can't write to file %s  !\n",o);
            break;
        case ERR_BADDECR:
            sprintf(errMess,"Error in file %s : error unpacking !\n",i);
            break;
        case ERR_UNKNMODE:
            sprintf(errMess,"Error in file %s : unknown compression mode used !\n",i);
            break;
        case ERR_NOPASSWD:
            sprintf(errMess,"Can't process file %s : file is encrypted !\n",i);
            break;
        case ERR_BADPASSWD:
            sprintf(errMess,"Error unpacking file %s . The password is probably wrong.\n",i);
            break;
        case ERR_FMS:
            sprintf(errMess,"Error in file %s : this file is not really a compressed disk image, but an FMS archive !\n",i);
            break;
        case ERR_GZIP:
            sprintf(errMess, "Can't convert file %s : gzip failed !\n", i);
            break;
        case ERR_READDISK:
            sprintf(errMess, "Can't extract files from %s : readdisk failed !\n", i);
            break;
        default:
            sprintf(errMess,"Error while processing file  %s : internal error !\n",i);
            break;
    }
}

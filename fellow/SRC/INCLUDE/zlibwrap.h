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

#include "defs.h"

BOOLE gzUnpack(const char *src, const char *dest);

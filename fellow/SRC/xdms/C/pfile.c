
/*
 *     xDMS  v1.3b  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Handles the processing of a single DMS archive
 *
 */


#define HEADLEN 56
#define THLEN 20
#define TRACK_BUFFER_LEN 32000
#define TEMP_BUFFER_LEN 32000


#include <stdio.h>

#ifdef _FELLOW_DEBUG_CRT_MALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#ifdef _FELLOW_DEBUG_CRT_MALLOC
#include <malloc.h>
#include <crtdbg.h>
#endif

#include <string.h>
#include <time.h>

#include "cdata.h"
#include "u_init.h"
#include "u_rle.h"
#include "u_quick.h"
#include "u_medium.h"
#include "u_deep.h"
#include "u_heavy.h"
#include "crc_csum.h"
#include "pfile.h"



static USHORT Process_Track(FILE *, FILE *, UCHAR *, UCHAR *, USHORT, USHORT);
static USHORT Unpack_Track(UCHAR *, UCHAR *, USHORT, USHORT, UCHAR, UCHAR);
static void dms_decrypt(UCHAR *, USHORT);


static char modes[7][7]={"NOCOMP","SIMPLE","QUICK ","MEDIUM","DEEP  ","HEAVY1","HEAVY2"};
static USHORT PWDCRC;

UCHAR *text;



USHORT Process_File(char *iname, char *oname, USHORT opt, USHORT PCRC, USHORT pwd){
	FILE *fi, *fo=NULL;
	USHORT from, to, geninfo, c_version, cmode, hcrc, disktype, ret;
	ULONG pkfsize, unpkfsize;
	UCHAR *b1, *b2;
	time_t date;


	b1 = (UCHAR *)calloc((size_t)TRACK_BUFFER_LEN,1);
	if (!b1) return ERR_NOMEMORY;
	b2 = (UCHAR *)calloc((size_t)TRACK_BUFFER_LEN,1);
	if (!b2) {
		free(b1);
		return ERR_NOMEMORY;
	}
	text = (UCHAR *)calloc((size_t)TEMP_BUFFER_LEN,1);
	if (!text) {
		free(b1);
		free(b2);
		return ERR_NOMEMORY;
	}

	fi = fopen(iname,"rb");
	if (!fi) {
		free(b1);
		free(b2);
		free(text);
		return ERR_CANTOPENIN;
	}

	if (fread(b1,1,HEADLEN,fi) != HEADLEN) {
		if (iname) fclose(fi);
		free(b1);
		free(b2);
		free(text);
		return ERR_SREAD;
	}

	if ( (b1[0] != 'D') || (b1[1] != 'M') || (b1[2] != 'S') || (b1[3] != '!') ) {
		/*  Check the first 4 bytes of file to see if it is "DMS!"  */
		if (iname) fclose(fi);
		free(b1);
		free(b2);
		free(text);
		return ERR_NOTDMS;
	}

	hcrc = (USHORT)((b1[HEADLEN-2]<<8) | b1[HEADLEN-1]);
	/* Header CRC */

	if (hcrc != CreateCRC(b1+4,(ULONG)(HEADLEN-6))) {
		if (iname) fclose(fi);
		free(b1);
		free(b2);
		free(text);
		return ERR_HCRC;
	}
	
	geninfo = (USHORT) ((b1[10]<<8) | b1[11]);	/* General info about archive */
	date = (time_t) ((((ULONG)b1[12])<<24) | (((ULONG)b1[13])<<16) | (((ULONG)b1[14])<<8) | (ULONG)b1[15]);	/* date in standard UNIX/ANSI format */
	from = (USHORT) ((b1[16]<<8) | b1[17]);		/*  Lowest track in archive. May be incorrect if archive is "appended" */
	to = (USHORT) ((b1[18]<<8) | b1[19]);		/*  Highest track in archive. May be incorrect if archive is "appended" */

	pkfsize = (ULONG) ((((ULONG)b1[21])<<16) | (((ULONG)b1[22])<<8) | (ULONG)b1[23]);	/*  Length of total packed data as in archive   */
	unpkfsize = (ULONG) ((((ULONG)b1[25])<<16) | (((ULONG)b1[26])<<8) | (ULONG)b1[27]);	/*  Length of unpacked data. Usually 901120 bytes  */

	c_version = (USHORT) ((b1[46]<<8) | b1[47]);	/*  version of DMS used to generate it  */
	disktype = (USHORT) ((b1[50]<<8) | b1[51]);		/*  Type of compressed disk  */
	cmode = (USHORT) ((b1[52]<<8) | b1[53]);        /*  Compression mode mostly used in this archive  */

	PWDCRC = PCRC;

	if (disktype == 7) {
		/*  It's not a DMS compressed disk image, but a FMS archive  */
		if (iname) fclose(fi);
		free(b1);
		free(b2);
		free(text);
		return ERR_FMS;
	}

	if ((geninfo & 2) && (!pwd))
		return ERR_NOPASSWD;

	fo = fopen(oname,"wb");
	if (!fo) {
		if (iname) fclose(fi);
		free(b1);
		free(b2);
		free(text);
		return ERR_CANTOPENOUT;
	}

	ret=NO_PROBLEM;

	Init_Decrunchers();

	while ( (ret=Process_Track(fi,fo,b1,b2,opt,(USHORT)((geninfo & 2)?pwd:0))) == NO_PROBLEM ) ;

	if (ret == DMS_FILE_END) ret = NO_PROBLEM;

	/*  Used to give an error message, but I have seen some DMS  */
	/*  files with texts or zeros at the end of the valid data   */
	/*  So, when we find something that is not a track header,   */
	/*  we suppose that the valid data is over. And say it's ok. */
	if (ret == ERR_NOTTRACK) ret = NO_PROBLEM;


	fclose(fi);
	fclose(fo);

	free(b1);
	free(b2);
	free(text);

	return ret;
}



static USHORT Process_Track(FILE *fi, FILE *fo, UCHAR *b1, UCHAR *b2, USHORT opt, USHORT pwd){
	USHORT hcrc, dcrc, usum, number, pklen1, pklen2, unpklen, l, r;
	UCHAR cmode, flags;

	l = (USHORT)fread(b1,1,THLEN,fi);

	if (l != THLEN) {
		if (l==0)
			return DMS_FILE_END;
		else
			return ERR_SREAD;
	}

	/*  "TR" identifies a Track Header  */
	if ((b1[0] != 'T')||(b1[1] != 'R')) return ERR_NOTTRACK;

	/*  Track Header CRC  */
	hcrc = (USHORT)((b1[THLEN-2] << 8) | b1[THLEN-1]);

	if (CreateCRC(b1,(ULONG)(THLEN-2)) != hcrc) return ERR_THCRC;

	number = (USHORT)((b1[2] << 8) | b1[3]);	/*  Number of track  */
	pklen1 = (USHORT)((b1[6] << 8) | b1[7]);	/*  Length of packed track data as in archive  */
	pklen2 = (USHORT)((b1[8] << 8) | b1[9]);	/*  Length of data after first unpacking  */
	unpklen = (USHORT)((b1[10] << 8) | b1[11]);	/*  Length of data after subsequent rle unpacking */
	flags = b1[12];		/*  control flags  */
	cmode = b1[13];		/*  compression mode used  */
	usum = (USHORT)((b1[14] << 8) | b1[15]);	/*  Track Data CheckSum AFTER unpacking  */
	dcrc = (USHORT)((b1[16] << 8) | b1[17]);	/*  Track Data CRC BEFORE unpacking  */

	if ((pklen1 > TRACK_BUFFER_LEN) || (pklen2 >TRACK_BUFFER_LEN) || (unpklen > TRACK_BUFFER_LEN)) return ERR_BIGTRACK;

	if (fread(b1,1,(size_t)pklen1,fi) != pklen1) return ERR_SREAD;

	if (CreateCRC(b1,(ULONG)pklen1) != dcrc) return ERR_TDCRC;

	/*  track 80 is FILEID.DIZ, track 0xffff (-1) is Banner  */
	/*  and track 0 with 1024 bytes only is a fake boot block with more advertising */
	/*  FILE_ID.DIZ is never encrypted  */

	if (pwd && (number!=80)) dms_decrypt(b1,pklen1);

	if ((number<80) && (unpklen>2048)) {
		r = Unpack_Track(b1, b2, pklen2, unpklen, cmode, flags);
		if (r != NO_PROBLEM) {
			if (pwd)
				return ERR_BADPASSWD;
			else
				return r;
		}
		if (usum != Calc_CheckSum(b2,(ULONG)unpklen)){
			if (pwd)
				return ERR_BADPASSWD;
			else
				return ERR_CSUM;
		}
		if (fwrite(b2,1,(size_t)unpklen,fo) != unpklen) return ERR_CANTWRITE;
		if (opt == OPT_VERBOSE) {
			fprintf(stderr,"#");
			fflush(stderr);
		}
	}
	return NO_PROBLEM;
}

static USHORT Unpack_Track(UCHAR *b1, UCHAR *b2, USHORT pklen2, USHORT unpklen, UCHAR cmode, UCHAR flags){
	switch (cmode){
		case 0:
			/*   No Compression   */
			memcpy(b2,b1,(size_t)unpklen);
			break;
		case 1:
			/*   Simple Compression   */
			if (Unpack_RLE(b1,b2,unpklen)) return ERR_BADDECR;
			break;
		case 2:
			/*   Quick Compression   */
			if (Unpack_QUICK(b1,b2,pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(b2,b1,unpklen)) return ERR_BADDECR;
			memcpy(b2,b1,(size_t)unpklen);
			break;
		case 3:
			/*   Medium Compression   */
			if (Unpack_MEDIUM(b1,b2,pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(b2,b1,unpklen)) return ERR_BADDECR;
			memcpy(b2,b1,(size_t)unpklen);
			break;
		case 4:
			/*   Deep Compression   */
			if (Unpack_DEEP(b1,b2,pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(b2,b1,unpklen)) return ERR_BADDECR;
			memcpy(b2,b1,(size_t)unpklen);
			break;
		case 5:
		case 6:
			/*   Heavy Compression   */
			if (cmode==5) {
				/*   Heavy 1   */
				if (Unpack_HEAVY(b1,b2,(UCHAR)(flags & 7),pklen2)) return ERR_BADDECR;
			} else {
				/*   Heavy 2   */
				if (Unpack_HEAVY(b1,b2,(UCHAR)(flags | 8),pklen2)) return ERR_BADDECR;
			}
			if (flags & 4) {
				/*  Unpack with RLE only if this flag is set  */
				if (Unpack_RLE(b2,b1,unpklen)) return ERR_BADDECR;
				memcpy(b2,b1,(size_t)unpklen);
			}
			break;
		default:
			return ERR_UNKNMODE;
	}

	if (!(flags & 1)) Init_Decrunchers();

	return NO_PROBLEM;

}


/*  DMS uses a lame encryption  */
static void dms_decrypt(UCHAR *p, USHORT len){
	USHORT t;

	while (len--){
		t = (USHORT) *p;
		*p++ ^= (UCHAR)PWDCRC;
		PWDCRC = (USHORT)((PWDCRC >> 1) + t);
	}
}

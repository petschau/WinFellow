/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Wav file sound dump                                                        */
/*                                                                            */
/* Authors: Petter Schau (peschau@online.no)                                  */
/*          Rainer Sinsch                                                     */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "defs.h"
#include "fellow.h"
#include "sound.h"
#include "graph.h"
#include "draw.h"



FILE *wav_FILE;
STR wav_filename[16];
ULO wav_serial;
sound_rates wav_rate;
ULO wav_rate_real;
BOOLE wav_stereo;
BOOLE wav_16bits;
ULO wav_filelength;
ULO wav_samplecount;
LON wav_samplesum;


/*============================================================*/
/* Add samples to device                                      */
/* We're not very concerned with performance here. After all, */
/* we're doing file output, which is likely to be slower than */
/* anything we can do on our own.                             */
/*============================================================*/

void wav8BitsMonoAdd(WOR *left, WOR *right, ULO sample_count) {
  ULO i;

  if (wav_FILE) {
    for (i = 0; i < sample_count; i++) {
      wav_samplesum = (((LON) left[i] + (LON) right[i])>>8) + 0x80;
      fwrite(&wav_samplesum, 1, 1, wav_FILE);
    }
    wav_filelength += sample_count;
  }
}

void wav8BitsStereoAdd(WOR *left, WOR *right, ULO sample_count) {
  ULO i;

  if (wav_FILE) {
    for (i = 0; i < sample_count; i++) {
      wav_samplesum = (((LON) left[i])>>8) + 0x80;
      fwrite(&wav_samplesum, 1, 1, wav_FILE);
      wav_samplesum = (((LON) right[i])>>8) + 0x80;
      fwrite(&wav_samplesum, 1, 1, wav_FILE);
    }
    wav_filelength += sample_count*2;
  }
}

void wav16BitsMonoAdd(WOR *left, WOR *right, ULO sample_count) {
  ULO i;

  if (wav_FILE) {
    for (i = 0; i < sample_count; i++) {
      wav_samplesum = left[i] + right[i];
      fwrite(&wav_samplesum, 2, 1, wav_FILE);
    }
    wav_filelength += sample_count*2;
  }
}

void wav16BitsStereoAdd(WOR *left, WOR *right, ULO sample_count) {
  ULO i;

  if (wav_FILE) {
    for (i = 0; i < sample_count; i++) {
      fwrite(&left[i], 2, 1, wav_FILE);
      fwrite(&right[i], 2, 1, wav_FILE);
    }
    wav_filelength += sample_count*4;
  }
}


/*===========================================================================*/
/* Write WAV header                                                          */
/*===========================================================================*/

void wavHeaderWrite(void) {
  static STR *wav_RIFF = {"RIFF"};
  static STR *wav_WAVEfmt = {"WAVEfmt "};
  static ULO wav_fmtchunklength = 16;
  static STR *wav_data = {"data"};
  ULO bytespersecond=wav_rate_real*(wav_stereo+1)*(wav_16bits+1);
  ULO bits = (wav_16bits + 1)*8;
  ULO blockalign = (wav_stereo + 1)*(wav_16bits + 1);

  /* This must still be wrong since wav-editors only reluctantly reads it */
  
  if ((wav_FILE = fopen(wav_filename, "wb")) != NULL) {
    wav_filelength = 36;
    fwrite(wav_RIFF, 4, 1, wav_FILE);           /* 0  RIFF signature */
    fwrite(&wav_filelength, 4, 1, wav_FILE);    /* 4  Length of file, that is, the number of bytes following the RIFF/Length pair */

    fwrite(wav_WAVEfmt, 8, 1, wav_FILE);        /* 8  Wave format chunk coming up */
    fwrite(&wav_fmtchunklength, 1, 4, wav_FILE);/* 16 Length of data in WAVEfmt chunk */

    /* Write a WAVEFORMATEX struct */

    fputc(0x01, wav_FILE); fputc(0, wav_FILE);  /* 20 Wave-format WAVE_FORMAT_PCM */
    fputc(wav_stereo + 1, wav_FILE);		/* 22 Channels in file */
    fputc(0, wav_FILE);
    fwrite(&wav_rate_real, 4, 1, wav_FILE);          /* 24 Samples per second */
    fwrite(&bytespersecond, 4, 1, wav_FILE);    /* 28 Average bytes per second */
    fwrite(&blockalign, 2, 1, wav_FILE);        /* 32 Block align */
    fwrite(&bits, 2, 1, wav_FILE);        /* 34 Bits per sample */
    fwrite(wav_data, 4, 1, wav_FILE);		/* 36 Data chunk */
    wav_filelength -= 36;
    fwrite(&wav_filelength, 4, 1, wav_FILE);    /* 40 Bytes in data chunk */
    wav_filelength += 36;
    fclose(wav_FILE);
    wav_FILE = NULL;
  }
}  

void wavLengthUpdate(void) {
  if (wav_FILE != NULL) {
    fseek(wav_FILE, 4, SEEK_SET);
    fwrite(&wav_filelength, 4, 1, wav_FILE);
    fseek(wav_FILE, 40, SEEK_SET);
    wav_filelength -= 36;
    fwrite(&wav_filelength, 4, 1, wav_FILE);
    wav_filelength += 36;
  }
}


/*===========================================================================*/
/* Set up WAV file                                                           */
/*===========================================================================*/

void wavFileInit(sound_rates rate, BOOLE bits16, BOOLE stereo) {
  if ((wav_rate != rate) ||
      (wav_16bits != bits16) ||
      (wav_stereo != stereo)) {
    sprintf(wav_filename, "FWAV%d.WAV", wav_serial++);
    wav_rate = rate;
    wav_rate_real = soundGetRateReal();
    wav_16bits = bits16;
    wav_stereo = stereo;
    wav_filelength = 0;
    wavHeaderWrite();
  }
  wav_FILE = fopen(wav_filename, "r+b");
  if (wav_FILE != NULL) fseek(wav_FILE, 0, SEEK_END);
}


/*===========================================================================*/
/* Play samples, or in this case, save it                                    */
/*===========================================================================*/

void wavPlay(WOR *left, WOR *right, ULO sample_count) {
  if (wav_stereo && wav_16bits)
    wav16BitsStereoAdd(left, right, sample_count);
  else if (!wav_stereo && wav_16bits)
    wav16BitsMonoAdd(left, right, sample_count);
  else if (wav_stereo && !wav_16bits)
    wav8BitsStereoAdd(left, right, sample_count);
  else
    wav8BitsMonoAdd(left, right, sample_count);
}

    
/*===========================================================================*/
/* Emulation start and stop                                                  */
/*                                                                           */
/* Called when wav dump is enabled                                           */
/* Will either continue with old file (same settings),                       */
/* or create new (changed settings)                                          */
/*===========================================================================*/

void wavEmulationStart(sound_rates rate,
		       BOOLE bits16,
		       BOOLE stereo,
		       ULO buffersamplecountmax) {
  wavFileInit(rate, bits16, stereo);
}

void wavEmulationStop(void) {
  wavLengthUpdate();
  if (wav_FILE != NULL) {
    fflush(wav_FILE);
    fclose(wav_FILE);
    wav_FILE = NULL;
  }
}


/*===========================================================================*/
/* Called once on startup                                                    */
/*                                                                           */
/* WAV supports any sound quality                                            */
/*===========================================================================*/

void wavStartup(void) {
  wav_serial = 0;
  wav_rate = (sound_rates) 9999;
  wav_16bits = 2;
  wav_stereo = 2; /* ILLEGAL */
  wav_FILE = NULL;
  wav_filelength = 0;
}


/*===========================================================================*/
/* Called once on shutdown                                                   */
/*===========================================================================*/

void wavShutdown(void) {
  if (wav_FILE != NULL)
    fclose(wav_FILE);
}



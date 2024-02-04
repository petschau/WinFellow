/*=========================================================================*/
/* Fellow                                                                  */
/* Send sound output to a wav                                              */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Rainer Sinsch                                                  */
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

#include <cstdio>
#include <sstream>
#include "CustomChipset/Sound/WavFileWriter.h"
#include "VirtualHost/Core.h"

using namespace std;
using namespace Service;

void WavFileWriter::Mono8BitsAdd(int16_t *left, int16_t *right, uint32_t sampleCount)
{
  if (_wavFile)
  {
    for (uint32_t i = 0; i < sampleCount; i++)
    {
      int32_t sampleSum = (((int32_t)left[i] + (int32_t)right[i]) >> 8) + 0x80;
      fwrite(&sampleSum, 1, 1, _wavFile);
    }
    _fileLength += sampleCount;
  }
}

void WavFileWriter::Stereo8BitsAdd(int16_t *left, int16_t *right, uint32_t sampleCount)
{
  if (_wavFile)
  {
    for (uint32_t i = 0; i < sampleCount; i++)
    {
      int32_t sampleSum = (((int32_t)left[i]) >> 8) + 0x80;
      fwrite(&sampleSum, 1, 1, _wavFile);
      sampleSum = (((int32_t)right[i]) >> 8) + 0x80;
      fwrite(&sampleSum, 1, 1, _wavFile);
    }
    _fileLength += sampleCount * 2;
  }
}

void WavFileWriter::Mono16BitsAdd(int16_t *left, int16_t *right, uint32_t sampleCount)
{
  if (_wavFile)
  {
    for (uint32_t i = 0; i < sampleCount; i++)
    {
      int32_t sampleSum = left[i] + right[i];
      fwrite(&sampleSum, 2, 1, _wavFile);
    }
    _fileLength += sampleCount * 2;
  }
}

void WavFileWriter::Stereo16BitsAdd(int16_t *left, int16_t *right, uint32_t sampleCount)
{
  if (_wavFile)
  {
    for (uint32_t i = 0; i < sampleCount; i++)
    {
      fwrite(&left[i], 2, 1, _wavFile);
      fwrite(&right[i], 2, 1, _wavFile);
    }
    _fileLength += sampleCount * 4;
  }
}

void WavFileWriter::HeaderWrite()
{
  static const char *wav_RIFF = {"RIFF"};
  static const char *wav_WAVEfmt = {"WAVEfmt "};
  static uint32_t wav_fmtchunklength = 16;
  static const char *wav_data = {"data"};
  uint32_t channelCount = _isStereo ? 2 : 1;
  uint32_t is16BitsMultipiler = _is16Bits ? 2 : 1;
  uint32_t bytespersecond = _rateReal * channelCount * is16BitsMultipiler;
  uint32_t bits = is16BitsMultipiler * 8;
  uint32_t blockalign = channelCount * is16BitsMultipiler;

  /* This must still be wrong since wav-editors only reluctantly reads it */

  if ((_wavFile = fopen(_filename, "wb")) != nullptr)
  {
    _fileLength = 36;
    fwrite(wav_RIFF, 4, 1, _wavFile);     /* 0  RIFF signature */
    fwrite(&_fileLength, 4, 1, _wavFile); /* 4  Length of file, that is, the number of bytes following the RIFF/Length pair */

    fwrite(wav_WAVEfmt, 8, 1, _wavFile);         /* 8  Wave format chunk coming up */
    fwrite(&wav_fmtchunklength, 1, 4, _wavFile); /* 16 Length of data in WAVEfmt chunk */

    /* Write a WAVEFORMATEX struct */

    fputc(0x01, _wavFile);
    fputc(0, _wavFile);            /* 20 Wave-format WAVE_FORMAT_PCM */
    fputc(channelCount, _wavFile); /* 22 Channels in file */
    fputc(0, _wavFile);
    fwrite(&_rateReal, 4, 1, _wavFile);      /* 24 Samples per second */
    fwrite(&bytespersecond, 4, 1, _wavFile); /* 28 Average bytes per second */
    fwrite(&blockalign, 2, 1, _wavFile);     /* 32 Block align */
    fwrite(&bits, 2, 1, _wavFile);           /* 34 Bits per sample */
    fwrite(wav_data, 4, 1, _wavFile);        /* 36 Data chunk */
    _fileLength -= 36;
    fwrite(&_fileLength, 4, 1, _wavFile); /* 40 Bytes in data chunk */
    _fileLength += 36;
    fclose(_wavFile);
    _wavFile = nullptr;
  }
}

void WavFileWriter::LengthUpdate()
{
  if (_wavFile != nullptr)
  {
    fseek(_wavFile, 4, SEEK_SET);
    fwrite(&_fileLength, 4, 1, _wavFile);
    fseek(_wavFile, 40, SEEK_SET);
    _fileLength -= 36;
    fwrite(&_fileLength, 4, 1, _wavFile);
    _fileLength += 36;
  }
}

void WavFileWriter::FileInit(sound_rates rate, bool is16Bits, bool isStereo, uint32_t sampleRate)
{
  char generic_wav_filename[256];

  if (_rate != rate || _is16Bits != is16Bits || _isStereo != isStereo)
  {

    char filename[FILEOPS_MAX_FILE_PATH];
    sprintf(filename, "FWAV%u.WAV", _serial++);

    _core.Fileops->GetGenericFileName(generic_wav_filename, "WinFellow", filename);
    strcpy(_filename, generic_wav_filename);

    _rate = rate;
    _rateReal = sampleRate;
    _is16Bits = is16Bits;
    _isStereo = isStereo;
    _fileLength = 0;
    HeaderWrite();
  }
  _wavFile = fopen(_filename, "r+b");
  if (_wavFile != nullptr) fseek(_wavFile, 0, SEEK_END);
}

void WavFileWriter::Play(int16_t *left, int16_t *right, uint32_t sampleCount)
{
  if (_isStereo)
  {
    if (_is16Bits)
    {
      Stereo16BitsAdd(left, right, sampleCount);
    }
    else
    {
      Stereo8BitsAdd(left, right, sampleCount);
    }
  }
  else
  {
    if (_is16Bits)
    {
      Mono16BitsAdd(left, right, sampleCount);
    }
    else
    {
      Mono8BitsAdd(left, right, sampleCount);
    }
  }
}

void WavFileWriter::EmulationStart(sound_rates rate, bool is16Bits, bool isStereo, uint32_t sampleRate)
{
  FileInit(rate, is16Bits, isStereo, sampleRate);
}

void WavFileWriter::EmulationStop()
{
  LengthUpdate();
  if (_wavFile != nullptr)
  {
    fflush(_wavFile);
    fclose(_wavFile);
    _wavFile = nullptr;
  }
}

WavFileWriter::WavFileWriter()
{
  _serial = 0;
  _rate = (sound_rates)9999;
  _is16Bits = false;
  _isStereo = false;
  _wavFile = nullptr;
  _fileLength = 0;
}

WavFileWriter::~WavFileWriter()
{
  if (_wavFile != nullptr) fclose(_wavFile);
}

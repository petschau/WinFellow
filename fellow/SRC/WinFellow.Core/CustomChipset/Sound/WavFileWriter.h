#pragma once

#include <cstdint>
#include <string>
#include "CustomChipset/Sound/SoundConfiguration.h"

class WavFileWriter
{
private:
  FILE *_wavFile;
  char _filename[256];
  uint32_t _serial;
  sound_rates _rate;
  uint32_t _rateReal;
  bool _isStereo;
  bool _is16Bits;
  uint32_t _fileLength;
  uint32_t _sampleCount;

  void Mono8BitsAdd(int16_t *left, int16_t *right, uint32_t sampleCount);
  void Stereo8BitsAdd(int16_t *left, int16_t *right, uint32_t sampleCount);
  void Mono16BitsAdd(int16_t *left, int16_t *right, uint32_t sampleCount);
  void Stereo16BitsAdd(int16_t *left, int16_t *right, uint32_t sampleCount);
  void HeaderWrite();
  void LengthUpdate();
  void FileInit(sound_rates rate, bool is16Bits, bool isStereo, uint32_t sampleRate);

public:
  void Play(int16_t *left, int16_t *right, uint32_t sampleCount);
  void EmulationStart(sound_rates rate, bool is16Bits, bool isStereo, uint32_t sampleRate);
  void EmulationStop();

  WavFileWriter();
  ~WavFileWriter();
};

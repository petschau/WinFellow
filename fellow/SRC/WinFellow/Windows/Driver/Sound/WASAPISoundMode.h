#pragma once

#include <cstdint>

/**
 * @brief Describes a supported WASAPI sound mode.
 * Supports 8, 16, and 32 bits per sample.
 */
struct WASAPISoundMode
{
  uint32_t Rate;              ///< Sample rate in Hz
  uint8_t BitsPerSample;      ///< Bits per sample (8, 16, or 32)
  bool IsStereo;              ///< True if stereo, false if mono
  uint32_t BufferSampleCount; ///< Number of samples in buffer
  uint32_t BufferBlockAlign;  ///< Block alignment in bytes
};
#pragma once

#include <cstdint>

class ISprites
{
public:
  virtual void NotifySprpthChanged(uint16_t data, unsigned int sprite_number) = 0;
  virtual void NotifySprptlChanged(uint16_t data, unsigned int sprite_number) = 0;
  virtual void NotifySprposChanged(uint16_t data, unsigned int sprite_number) = 0;
  virtual void NotifySprctlChanged(uint16_t data, unsigned int sprite_number) = 0;
  virtual void NotifySprdataChanged(uint16_t data, unsigned int sprite_number) = 0;
  virtual void NotifySprdatbChanged(uint16_t data, unsigned int sprite_number) = 0;

  virtual void EndOfLine(uint32_t rasterY) = 0;
  virtual void EndOfFrame() = 0;
  virtual void HardReset() = 0;
  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;

  virtual ~ISprites() = default;
};

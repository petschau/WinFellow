#ifndef SPRITE_H
#define SPRITE_H

#include "graph.h"

extern void spriteInitializeFromEmulationMode();
extern void spriteEndOfLine(ULO rasterY);
extern void spriteEndOfFrame();
extern void spriteHardReset();
extern void spriteEmulationStart();
extern void spriteEmulationStop();
extern void spriteStartup();
extern void spriteShutdown();

class Sprites
{
public:
  virtual void NotifySprpthChanged(UWO data, unsigned int sprite_number) = 0;
  virtual void NotifySprptlChanged(UWO data, unsigned int sprite_number) = 0;
  virtual void NotifySprposChanged(UWO data, unsigned int sprite_number) = 0;
  virtual void NotifySprctlChanged(UWO data, unsigned int sprite_number) = 0;
  virtual void NotifySprdataChanged(UWO data, unsigned int sprite_number) = 0;
  virtual void NotifySprdatbChanged(UWO data, unsigned int sprite_number) = 0;

  virtual void EndOfLine(ULO rasterY) = 0;
  virtual void EndOfFrame() = 0;
  virtual void HardReset() = 0;
  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;

  virtual ~Sprites() = default;
};

class LineExactSprites;
class CycleExactSprites;

extern Sprites *sprites;
extern LineExactSprites *line_exact_sprites;
extern CycleExactSprites *cycle_exact_sprites;

#endif /* SPRITE_H */

#include "DEFS.H"
#include "FELLOW.H"
#include "GRAPH.H"
#include "draw_interlace_control.h"

typedef struct
{
  bool frame_is_interlaced;
  bool frame_is_long;
} draw_interlace_status;

draw_interlace_status interlace_status;

bool drawGetFrameIsInterlaced(void)
{
  return interlace_status.frame_is_interlaced;
}

bool drawGetFrameIsLong(void)
{
  return interlace_status.frame_is_long;
}

void drawChangeInterlaceStatus(bool lace_bit)
{
  interlace_status.frame_is_interlaced = lace_bit;

  fellowAddLog("Frames are now %s\n", (lace_bit) ? "interlaced" : "normal");
}

bool drawDecideInterlaceStatusForNextFrame(void)
{
  bool lace_bit = ((bplcon0 & 4) == 4);
  bool interlace_status_changed = (lace_bit != interlace_status.frame_is_interlaced);

  if (interlace_status_changed)
  {
    drawChangeInterlaceStatus(lace_bit);
  }

  if (interlace_status.frame_is_interlaced)
  {
    lof = lof ^ 0x8000;
    interlace_status.frame_is_long = ((lof & 0x8000) == 0x8000);
  }
  return interlace_status_changed;
}

void drawClearInterlaceStatus(void)
{
  interlace_status.frame_is_interlaced = false;
}

#include "DEFS.H"
#include "FELLOW.H"
#include "BUS.H"
#include "GRAPH.H"
#include "DRAW.H"
#include "draw_interlace_control.h"

typedef struct
{
  bool frame_is_interlaced;
  bool frame_is_long;
  bool enable_deinterlace;
  bool use_interlaced_rendering;
} draw_interlace_status;

draw_interlace_status interlace_status;

bool drawGetUseInterlacedRendering(void)
{
  return interlace_status.use_interlaced_rendering;
}

bool drawGetFrameIsLong(void)
{
  return interlace_status.frame_is_long;
}

bool drawDecideUseInterlacedRendering(void)
{
  return interlace_status.enable_deinterlace && interlace_status.frame_is_interlaced;
}

void drawDecideInterlaceStatusForNextFrame(void)
{
  bool lace_bit = ((bplcon0 & 4) == 4);

  interlace_status.frame_is_interlaced = lace_bit;
  if (interlace_status.frame_is_interlaced)
  {
    // Automatic long / short frame toggeling
    lof = lof ^ 0x8000;
  }

  interlace_status.frame_is_long = ((lof & 0x8000) == 0x8000);
  busSetScreenLimits(interlace_status.frame_is_long);

  bool use_interlaced_rendering = drawDecideUseInterlacedRendering();
  if (use_interlaced_rendering != interlace_status.use_interlaced_rendering)
  {

    if ((drawGetDisplayScaleStrategy() == DISPLAYSCALE_STRATEGY_SCANLINES) &&
        interlace_status.use_interlaced_rendering)
    {
      // Clear buffers when switching back to scanlines from interlaced rendering
      // to avoid a ghost image remaining in the scanlines.
      draw_clear_buffers = drawGetBufferCount();
    }

    interlace_status.use_interlaced_rendering = use_interlaced_rendering;
    drawReinitializeRendering();
  }

//    fellowAddLog("Frames are %s, frame no %I64d\n", (interlace_status.frame_is_long) ? "long" : "short", busGetRasterFrameCount());
}

void drawInterlaceStartup(void)
{
  interlace_status.frame_is_interlaced = false;
  interlace_status.frame_is_long = true;
  interlace_status.enable_deinterlace = true;
  interlace_status.use_interlaced_rendering = false;
}

void drawInterlaceEndOfFrame(void)
{
  drawDecideInterlaceStatusForNextFrame();
}

void drawSetDeinterlace(bool deinterlace)
{
  interlace_status.enable_deinterlace = deinterlace;
}
/*=========================================================================*/
/* Fellow                                                                  */
/* Convert graphics data to temporary format suitable for fast             */
/* drawing on a chunky display.                                            */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Worfje                                                         */
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
#include "Defs.h"
#include "FellowMain.h"
#include "chipset.h"
#include "Renderer.h"
#include "MemoryInterface.h"
#include "Keyboard.h"
#include "Blitter.h"
#include "GraphicsPipeline.h"
#include "Renderer.h"
#include "GraphicsPipeline.h"
#include "CustomChipset/Sprite/LineExactSprites.h"
#include "CpuIntegration.h"
#include "draw_interlace_control.h"

#include "graphics/Graphics.h"

/*======================================================================*/
/* flag that handles loss of surface content due to DirectX malfunction */
/*======================================================================*/

BOOLE graph_buffer_lost;

/*===========================================================================*/
/* Lookup-tables for planar to chunky routines                               */
/*===========================================================================*/

planar2chunkyroutine graph_decode_line_ptr;
planar2chunkyroutine graph_decode_line_tab[16];
planar2chunkyroutine graph_decode_line_dual_tab[16];

/*===========================================================================*/
/* Planar to chunky lookuptables for data                                    */
/*===========================================================================*/

uint32_t graph_deco1[256][2];
uint32_t graph_deco2[256][2];
uint32_t graph_deco3[256][2];
uint32_t graph_deco4[256][2];
uint32_t graph_deco5[256][2];
uint32_t graph_deco6[256][2];

uint8_t graph_line1_tmp[1024];
uint8_t graph_line2_tmp[1024];

/*===========================================================================*/
/* Line description registers and data                                       */
/*===========================================================================*/

uint32_t graph_DDF_start;
uint32_t graph_DDF_word_count;
uint32_t graph_DIW_first_visible;
uint32_t graph_DIW_last_visible;

/*===========================================================================*/
/* Translation tables for colors                                             */
/*===========================================================================*/

uint32_t graph_color_shadow[64]; /* Colors corresponding to the different Amiga- */
/* registers. Initialized from draw_color_table */
/* whenever WCOLORXX is written.                */

uint16_t graph_color[64]; /* Copy of Amiga version of colors */
BOOLE graph_playfield_on;

/*===========================================================================*/
/* IO-registers                                                              */
/*===========================================================================*/

uint32_t bpl1pt, bpl2pt, bpl3pt, bpl4pt, bpl5pt, bpl6pt;
uint32_t lof, ddfstrt, ddfstop, bplcon1, bpl1mod, bpl2mod;
uint32_t evenscroll, evenhiscroll, oddscroll, oddhiscroll;
uint32_t diwstrt, diwstop;
uint32_t diwxleft, diwxright, diwytop, diwybottom;
uint32_t dmacon;

/*===========================================================================*/
/* Clear the line descriptions                                               */
/* This function is called on emulation start and from the debugger when     */
/* the emulator is stopped mid-line. It must create a valid dummy array.     */
/*===========================================================================*/

void graphLineDescClear()
{
  memset(graph_frame, 0, sizeof(graph_line) * 3 * 628);
  for (int frame = 0; frame < 3; frame++)
  {
    for (int line = 0; line < 628; line++)
    {
      graph_frame[frame][line].linetype = graph_linetypes::GRAPH_LINE_BG;
      graph_frame[frame][line].draw_line_routine = (void *)draw_line_BG_routine;
      graph_frame[frame][line].colors[0] = 0;
      graph_frame[frame][line].frames_left_until_BG_skip = drawGetBufferCount(); /* ie. one more than normal to draw once in each buffer */
      graph_frame[frame][line].sprite_ham_slot = 0xffffffff;
      graph_frame[frame][line].has_ham_sprites_online = false;
    }
  }
}

graph_line *graphGetLineDesc(int buffer_no, int currentY)
{
  int line_index = currentY * 2;

  if (drawGetUseInterlacedRendering() && !drawGetFrameIsLong())
  {
    line_index += 1;
  }
  return &graph_frame[buffer_no][line_index];
}

/*===========================================================================*/
/* The mapping from Amiga colors to host colors are dependent on the host    */
/* resolution. We need to recalculate the colors to reflect the possibly new */
/* color format.                                                             */
/* This function is called from the drawEmulationStart() functions since we  */
/* don't know the color format yet in graphEmulationStart()                  */
/*===========================================================================*/

void graphInitializeShadowColors()
{
  for (uint32_t i = 0; i < 64; i++)
    graph_color_shadow[i] = draw_color_table[graph_color[i] & 0xfff];
}

/*===========================================================================*/
/* Clear all register data                                                   */
/*===========================================================================*/

static void graphIORegistersClear()
{
  for (uint32_t i = 0; i < 64; i++)
    graph_color_shadow[i] = graph_color[i] = 0;
  graph_playfield_on = FALSE;
  lof = 0x8000; /* Long frame */
  bpl1mod = 0;
  bpl2mod = 0;
  bpl1pt = 0;
  bpl2pt = 0;
  bpl3pt = 0;
  bpl4pt = 0;
  bpl5pt = 0;
  bpl6pt = 0;
  _core.Registers.BplCon0 = 0;
  bplcon1 = 0;
  _core.Registers.BplCon2 = 0;
  ddfstrt = 0;
  ddfstop = 0;
  graph_DDF_start = 0;
  graph_DDF_word_count = 0;
  diwstrt = 0;
  diwstop = 0;
  diwxleft = 0;
  diwxright = 0;
  diwytop = 0;
  diwybottom = 0;
  graph_DIW_first_visible = 256;
  graph_DIW_last_visible = 256;
  evenscroll = 0;
  evenhiscroll = 0;
  oddscroll = 0;
  oddhiscroll = 0;
  _core.Registers.DmaConR = 0;
  dmacon = 0;
}

/*===========================================================================*/
/* Graphics Register Access routines                                         */
/*===========================================================================*/

/*===========================================================================*/
/* DMACONR - $dff002 Read                                                    */
/*===========================================================================*/

uint16_t rdmaconr(uint32_t address)
{
  if (blitterGetZeroFlag())
  {
    return (uint16_t)(_core.Registers.DmaConR | 0x00002000);
  }
  return (uint16_t)_core.Registers.DmaConR;
}

/*===========================================================================*/
/* VPOSR - $dff004 Read vpos and chipset ID bits                             */
/*===========================================================================*/

uint32_t graphAdjustVPosY(uint32_t y, uint32_t x)
{
  if (x <= 1 && y > 0)
  {
    y--;
  }
  return y;
}

uint16_t rvposr(uint32_t address)
{
  uint32_t y = graphAdjustVPosY(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  if (chipsetGetECS())
  {
    return (uint16_t)((lof | (y >> 8)) | 0x2000);
  }
  return (uint16_t)(lof | (y >> 8));
}

/*===========================================================================*/
/* VHPOSR - $dff006 Read                                                     */
/*===========================================================================*/

uint16_t rvhposr(uint32_t address)
{
  uint32_t x = _core.Timekeeper->GetAgnusLineCycle();
  uint32_t y = graphAdjustVPosY(_core.Timekeeper->GetAgnusLine(), x);
  return (uint16_t)(x | ((y & 0xFF) << 8));
}

/*===========================================================================*/
/* ID - $dff07c Read                                                         */
/*                                                                           */
/* return 0xffff                                                             */
/*===========================================================================*/

uint16_t rid(uint32_t address)
{
  return 0xFFFF;
}

/*===========================================================================*/
/* VPOS - $dff02a Write                                                      */
/*                                                                           */
/* lof = data & 0x8000;                                                      */
/*===========================================================================*/

void wvpos(uint16_t data, uint32_t address)
{
  lof = (uint32_t)(data & 0x8000);
}

/*===========================================================================*/
/* DIWSTRT - $dff08e Write                                                   */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

void wdiwstrt(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    uint32_t diwstrt_old = diwstrt;
    diwstrt = data;
    if (diwstrt_old != diwstrt)
    {
      GraphicsContext.DIWXStateMachine.ChangedValue();
      GraphicsContext.DIWYStateMachine.ChangedValue();
    }
  }

  diwstrt = data;
  diwytop = (data >> 8) & 0x000000FF;
  if (diwxright == 472)
  {
    diwxleft = 88;
  }
  else
  {
    diwxleft = data & 0x000000FF;
  }
  graphCalculateWindow();
  graphPlayfieldOnOff();
}

/*===========================================================================*/
/* DIWSTOP - $dff090 Write                                                   */
/*===========================================================================*/

void wdiwstop(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    uint32_t diwstop_old = diwstop;
    diwstop = data;
    if (diwstop_old != diwstop)
    {
      GraphicsContext.DIWXStateMachine.ChangedValue();
      GraphicsContext.DIWYStateMachine.ChangedValue();
    }
  }

  diwstop = data;
  if ((((data >> 8) & 0xff) & 0x80) == 0x0)
  {
    diwybottom = ((data >> 8) & 0xff) + 256;
  }
  else
  {
    diwybottom = (data >> 8) & 0xff;
  }

  if (((data & 0xff) ^ 0x100) < 457)
  {
    diwxleft = diwstrt & 0xff;
    diwxright = (data & 0xff) ^ 0x100;
  }
  else
  {
    diwxleft = 88;
    diwxright = 472;
  }
  graphCalculateWindow();
  graphPlayfieldOnOff();
}

/*==============================================================================*/
/* DDFSTRT - $dff092 Write                                                      */
/*                                                                              */
/* When this value is written, the scroll (BPLCON1) also need to be reevaluated */
/* for _reversal_ of the scroll values.                                         */
/* _wbplcon1_ calls _graphCalculateWindow_ so we don't need to here             */
/*==============================================================================*/

void wddfstrt(uint16_t data, uint32_t address)
{
  uint32_t ddfstrt_old;

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    ddfstrt_old = ddfstrt;
  }

  if ((data & 0xfc) < 0x18)
  {
    ddfstrt = 0x18;
  }
  else
  {
    ddfstrt = data & 0xfc;
  }
  wbplcon1((uint16_t)bplcon1, address);

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    if (ddfstrt_old != ddfstrt)
    {
      GraphicsContext.DDFStateMachine.ChangedValue();
    }
  }
}

/*==============================================================================*/
/* DDFSTOP - $dff094 Write                                                      */
/*                                                                              */
/* These registers control the horizontal timing of the                         */
/* beginning and end of the bitplane DMA display data fetch.                    */
/* (quote from 'Amiga Hardware Reference Manual')                               */
/*==============================================================================*/

void wddfstop(uint16_t data, uint32_t address)
{
  uint32_t ddfstop_old;

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    ddfstop_old = ddfstop;
  }

  if ((data & 0xfc) > 0xd8)
  {
    ddfstop = 0xd8;
  }
  else
  {
    ddfstop = data & 0xfc;
  }
  graphCalculateWindow();

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    if (ddfstop_old != ddfstop)
    {
      GraphicsContext.DDFStateMachine.ChangedValue();
    }
  }
}

/*===========================================================================*/
/* DMACON - $dff096 Write                                                    */
/*                                                                           */
/* $dff096  - Read from $dff002                                              */
/* dmacon - zero if master dma bit is off                                    */
/* dmaconr - is always correct.                                              */
/*===========================================================================*/

void wdmacon(uint16_t data, uint32_t address)
{
  uint32_t prev_dmacon;

  // check SET/CLR bit is 1 or 0
  if ((data & 0x8000) != 0x0)
  {
    // SET/CLR bit is 1 (bits set to 1 will set bits)
    uint32_t local_data = data & 0x7ff; // zero bits 15 to 11 (readonly bits and set/clr bit)
    // test if BLTPRI got turned on (blitter DMA priority)
    if ((local_data & 0x0400) != 0x0)
    {
      // BLTPRI bit is on now, was it turned off before?
      if (!_core.RegisterUtility.IsBlitterPriorityEnabled())
      {
        // BLTPRI was turned off before and therefor
        // BLTPRI got turned on, stop CPU until a blit is
        // finished if this is a blit that uses all cycles
        if (_core.Events->blitterEvent.IsEnabled())
        {
          if (blitterGetFreeCycles() == 0)
          {
            // below delays CPU additionally cycles
            cpuIntegrationSetChipCycles(cpuIntegrationGetChipCycles() + (_core.Events->blitterEvent.cycle - _core.Timekeeper->GetFrameCycle()));
          }
        }
      }
    }

    _core.Registers.DmaConR |= local_data;
    prev_dmacon = dmacon; // stored in edx
    if (_core.RegisterUtility.IsMasterDMAEnabled())
    {
      dmacon = _core.Registers.DmaConR;
    }
    else
    {
      dmacon = 0;
    }

    // enable audio channel X ?
    for (unsigned int i = 0; i < 4; i++)
    {
      if (((dmacon & (1 << i))) != 0x0)
      {
        // audio channel 0 DMA enable bit is set now
        if ((prev_dmacon & (1 << i)) == 0x0)
        {
          // audio channel 0 DMA enable bit was clear previously
          _core.Sound->ChannelEnable(i);
        }
      }
    }

    // check if already a call to wbltsize was executed
    // before the blitter DMA channel was activated
    if (blitterGetDMAPending())
    {
      if ((dmacon & 0x0040) != 0x0)
      {
        blitterCopy();
      }
    }

    _core.CurrentCopper->NotifyDMAEnableChanged((dmacon & 0x80) == 0x80);
  }
  else
  {
    // SET/CLR bit is 0 (bits set to 1 will clear bits)
    _core.Registers.DmaConR = (~(data & 0x07ff)) & _core.Registers.DmaConR;
    prev_dmacon = dmacon;
    if (_core.RegisterUtility.IsMasterDMAEnabled())
    {
      dmacon = _core.Registers.DmaConR;
    }
    else
    {
      dmacon = 0;
    }

    // if a blitter DMA is turned off in the middle of the blit action
    // we finish the blit
    if ((dmacon & 0x0040) == 0x0)
    {
      if (blitterIsStarted())
      {
        blitForceFinish();
      }
    }

    // disable audio channel X ?
    for (unsigned int i = 0; i < 4; i++)
    {
      if ((dmacon & (1 << i)) == 0x0)
      {
        if ((prev_dmacon & (1 << i)) != 0x0)
        {
          _core.Sound->ChannelKill(i);
        }
      }
    }

    // check if already a call to wbltsize was executed
    // before the blitter DMA channel was activated
    if (blitterGetDMAPending())
    {
      if ((dmacon & 0x0040) != 0x0)
      {
        blitterCopy();
      }
    }

    _core.CurrentCopper->NotifyDMAEnableChanged((dmacon & 0x80) == 0x80);
  }
}

/*===========================================================================*/
/* BPL1PTH - $dff0e0 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl1pth(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl1pt = chipsetReplaceHighPtr(bpl1pt, data);
}

/*===========================================================================*/
/* BPL1PTL - $dff0e2 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl1ptl(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl1pt = chipsetReplaceLowPtr(bpl1pt, data);
}

/*===========================================================================*/
/* BPL2PTH - $dff0e4 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl2pth(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl2pt = chipsetReplaceHighPtr(bpl2pt, data);
}

/*===========================================================================*/
/* BPL2PTL - $dff0e6 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl2ptl(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl2pt = chipsetReplaceLowPtr(bpl2pt, data);
}

/*===========================================================================*/
/* BPL3PTH - $dff0e8 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl3pth(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
  bpl3pt = chipsetReplaceHighPtr(bpl3pt, data);
}

/*===========================================================================*/
/* BPL3PTL - $dff0eA Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl3ptl(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl3pt = chipsetReplaceLowPtr(bpl3pt, data);
}

/*===========================================================================*/
/* BPL4PTH - $dff0e8 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl4pth(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl4pt = chipsetReplaceHighPtr(bpl4pt, data);
}

/*===========================================================================*/
/* BPL4PTL - $dff0eA Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl4ptl(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl4pt = chipsetReplaceLowPtr(bpl4pt, data);
}

/*===========================================================================*/
/* BPL5PTH - $dff0e8 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl5pth(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl5pt = chipsetReplaceHighPtr(bpl5pt, data);
}

/*===========================================================================*/
/* BPL5PTL - $dff0eA Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl5ptl(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl5pt = chipsetReplaceLowPtr(bpl5pt, data);
}

/*===========================================================================*/
/* BPL3PTH - $dff0e8 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl6pth(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl6pt = chipsetReplaceHighPtr(bpl6pt, data);
}

/*===========================================================================*/
/* BPL6PTL - $dff0eA Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl6ptl(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());

  bpl6pt = chipsetReplaceLowPtr(bpl6pt, data);
}

/*===========================================================================*/
/* BPLCON0 - $dff100 Write                                                   */
/*===========================================================================*/

void wbplcon0(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    if (_core.Registers.BplCon0 != data)
    {
      GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    }
  }

  _core.Registers.BplCon0 = data;
  uint32_t local_data = (data >> 12) & 0x0f;

  // check if DBLPF bit is set
  if (_core.RegisterUtility.IsDualPlayfieldEnabled())
  {
    // double playfield, select a decoding function
    // depending on hires and playfield priority
    graph_decode_line_ptr = graph_decode_line_dual_tab[local_data];
  }
  else
  {
    graph_decode_line_ptr = graph_decode_line_tab[local_data];
  }

  // check if HOMOD bit is set
  if (_core.RegisterUtility.IsHAMEnabled())
  {
    // hold-and-modify mode
    draw_line_BPL_res_routine = draw_line_HAM_lores_routine;
  }
  else
  {
    // check if DBLPF bit is set
    if (_core.RegisterUtility.IsDualPlayfieldEnabled())
    {
      // check if HIRES is set
      if (_core.RegisterUtility.IsHiresEnabled())
      {
        draw_line_BPL_res_routine = draw_line_dual_hires_routine;
      }
      else
      {
        draw_line_BPL_res_routine = draw_line_dual_lores_routine;
      }
    }
    else
    {
      // check if HIRES is set
      if (_core.RegisterUtility.IsHiresEnabled())
      {
        draw_line_BPL_res_routine = draw_line_hires_routine;
      }
      else
      {
        draw_line_BPL_res_routine = draw_line_lores_routine;
      }
    }
  }
  graphCalculateWindow();
}

// when ddfstrt is (mod 8)+4, shift order is 8-f,0-7 (lores) (Example: New Zealand Story)
// when ddfstrt is (mod 8)+2, shift order is 4-7,0-3 (hires)

/*===========================================================================*/
/* BPLCON1 - $dff102 Write                                                   */
/*                                                                           */
/* extra variables                                                           */
/* oddscroll - dword with the odd lores scrollvalue                          */
/* evenscroll - dword with the even lores scrollvalue                         */
/*                                                                           */
/* oddhiscroll - dword with the odd hires scrollvalue                        */
/* evenhiscroll - dword with the even hires scrollvalue                      */
/*===========================================================================*/

void wbplcon1(uint16_t data, uint32_t address)
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    if (bplcon1 != (data & 0xff))
    {
      GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    }
  }

  bplcon1 = data & 0xff;

  // check for reverse shift order
  if ((ddfstrt & 0x04) != 0)
  {
    oddscroll = ((bplcon1 & 0x0f) + 8) & 0x0f;
  }
  else
  {
    oddscroll = bplcon1 & 0x0f;
  }

  // check for ?
  if ((ddfstrt & 0x02) != 0)
  {
    oddhiscroll = (((oddscroll & 0x07) + 4) & 0x07) << 1;
  }
  else
  {
    oddhiscroll = (oddscroll & 0x07) << 1;
  }

  // check for reverse shift order
  if ((ddfstrt & 0x04) != 0)
  {
    evenscroll = (((bplcon1 & 0xf0) >> 4) + 8) & 0x0f;
  }
  else
  {
    evenscroll = (bplcon1 & 0xf0) >> 4;
  }

  // check for ?
  if ((ddfstrt & 0x02) != 0)
  {
    evenhiscroll = (((evenscroll & 0x07) + 4) & 0x07) << 1;
  }
  else
  {
    evenhiscroll = (evenscroll & 0x07) << 1;
  }

  graphCalculateWindow();
}

/*===========================================================================*/
/* BPLCON2 - $dff104 Write                                                   */
/*===========================================================================*/

void wbplcon2(uint16_t data, uint32_t address)
{
  _core.Registers.BplCon2 = data;
}

/*===========================================================================*/
/* BPL1MOD - $dff108 Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl1mod(uint16_t data, uint32_t address)
{
  uint32_t new_value = (uint32_t)(int32_t)(int16_t)(data & 0xfffe);
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    if (bpl1mod != new_value)
    {
      GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    }
  }
  bpl1mod = new_value;
}

/*===========================================================================*/
/* BPL2MOD - $dff10a Write                                                   */
/*                                                                           */
/*===========================================================================*/

void wbpl2mod(uint16_t data, uint32_t address)
{
  uint32_t new_value = (uint32_t)(int32_t)(int16_t)(data & 0xfffe);
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    if (bpl2mod != new_value)
    {
      GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    }
  }
  bpl2mod = new_value;
}

/*===========================================================================*/
/* COLOR - $dff180 to $dff1be                                                */
/*                                                                           */
/*===========================================================================*/

void wcolor(uint16_t data, uint32_t address)
{
  uint32_t color_index = ((address & 0x1ff) - 0x180) >> 1;

  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    if (graph_color[color_index] != (data & 0x0fff))
    {
      GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    }
  }

  // normal mode
  graph_color[color_index] = (uint16_t)(data & 0x0fff);
  graph_color_shadow[color_index] = draw_color_table[data & 0xfff];
  // half bright mode
  graph_color[color_index + 32] = (uint16_t)(((data & 0xfff) & 0xeee) >> 1);
  graph_color_shadow[color_index + 32] = draw_color_table[(((data & 0xfff) & 0xeee) >> 1)];
}

/*===========================================================================*/
/* Registers the graphics IO register handlers                               */
/*===========================================================================*/

void graphIOHandlersInstall()
{
  memorySetIoReadStub(0x002, rdmaconr);
  memorySetIoReadStub(0x004, rvposr);
  memorySetIoReadStub(0x006, rvhposr);
  memorySetIoReadStub(0x07c, rid);
  memorySetIoWriteStub(0x02a, wvpos);
  memorySetIoWriteStub(0x08e, wdiwstrt);
  memorySetIoWriteStub(0x090, wdiwstop);
  memorySetIoWriteStub(0x092, wddfstrt);
  memorySetIoWriteStub(0x094, wddfstop);
  memorySetIoWriteStub(0x096, wdmacon);
  memorySetIoWriteStub(0x0e0, wbpl1pth);
  memorySetIoWriteStub(0x0e2, wbpl1ptl);
  memorySetIoWriteStub(0x0e4, wbpl2pth);
  memorySetIoWriteStub(0x0e6, wbpl2ptl);
  memorySetIoWriteStub(0x0e8, wbpl3pth);
  memorySetIoWriteStub(0x0ea, wbpl3ptl);
  memorySetIoWriteStub(0x0ec, wbpl4pth);
  memorySetIoWriteStub(0x0ee, wbpl4ptl);
  memorySetIoWriteStub(0x0f0, wbpl5pth);
  memorySetIoWriteStub(0x0f2, wbpl5ptl);
  memorySetIoWriteStub(0x0f4, wbpl6pth);
  memorySetIoWriteStub(0x0f6, wbpl6ptl);
  memorySetIoWriteStub(0x100, wbplcon0);
  memorySetIoWriteStub(0x102, wbplcon1);
  memorySetIoWriteStub(0x104, wbplcon2);
  memorySetIoWriteStub(0x108, wbpl1mod);
  memorySetIoWriteStub(0x10a, wbpl2mod);
  for (uint32_t i = 0x180; i < 0x1c0; i += 2)
    memorySetIoWriteStub(i, wcolor);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 to 4 (or 6) bitplanes hires (or lores)     */
/*===========================================================================*/

// Decode the odd part of the first 4 pixels
static __inline uint32_t graphDecodeOdd1(int bitplanes, uint32_t dat1, uint32_t dat3, uint32_t dat5)
{
  switch (bitplanes)
  {
    case 1:
    case 2: return graph_deco1[dat1][0];
    case 3:
    case 4: return graph_deco1[dat1][0] | graph_deco3[dat3][0];
    case 5:
    case 6: return graph_deco1[dat1][0] | graph_deco3[dat3][0] | graph_deco5[dat5][0];
  }
  return 0;
}

// Decode the odd part of the last 4 pixels
static __inline uint32_t graphDecodeOdd2(int bitplanes, uint32_t dat1, uint32_t dat3, uint32_t dat5)
{
  switch (bitplanes)
  {
    case 1:
    case 2: return graph_deco1[dat1][1];
    case 3:
    case 4: return graph_deco1[dat1][1] | graph_deco3[dat3][1];
    case 5:
    case 6: return graph_deco1[dat1][1] | graph_deco3[dat3][1] | graph_deco5[dat5][1];
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline uint32_t graphDecodeEven1(int bitplanes, uint32_t dat2, uint32_t dat4, uint32_t dat6)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco2[dat2][0];
    case 4:
    case 5: return graph_deco2[dat2][0] | graph_deco4[dat4][0];
    case 6: return graph_deco2[dat2][0] | graph_deco4[dat4][0] | graph_deco6[dat6][0];
  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline uint32_t graphDecodeEven2(int bitplanes, uint32_t dat2, uint32_t dat4, uint32_t dat6)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco2[dat2][1];
    case 4:
    case 5: return graph_deco2[dat2][1] | graph_deco4[dat4][1];
    case 6: return graph_deco2[dat2][1] | graph_deco4[dat4][1] | graph_deco6[dat6][1];
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline uint32_t graphDecodeDualOdd1(int bitplanes, uint32_t datA, uint32_t datB, uint32_t datC)
{
  switch (bitplanes)
  {
    case 1:
    case 2: return graph_deco1[datA][0];
    case 3:
    case 4: return graph_deco1[datA][0] | graph_deco2[datB][0];
    case 5:
    case 6: return graph_deco1[datA][0] | graph_deco2[datB][0] | graph_deco3[datC][0];
  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline uint32_t graphDecodeDualOdd2(int bitplanes, uint32_t datA, uint32_t datB, uint32_t datC)
{
  switch (bitplanes)
  {
    case 1:
    case 2: return graph_deco1[datA][1];
    case 3:
    case 4: return graph_deco1[datA][1] | graph_deco2[datB][1];
    case 5:
    case 6: return graph_deco1[datA][1] | graph_deco2[datB][1] | graph_deco3[datC][1];
  }
  return 0;
}

// Decode the even part of the first 4 pixels
static __inline uint32_t graphDecodeDualEven1(int bitplanes, uint32_t datA, uint32_t datB, uint32_t datC)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco1[datA][0];
    case 4:
    case 5: return graph_deco1[datA][0] | graph_deco2[datB][0];
    case 6: return graph_deco1[datA][0] | graph_deco2[datB][0] | graph_deco3[datC][0];
  }
  return 0;
}

// Decode the even part of the last 4 pixels
static __inline uint32_t graphDecodeDualEven2(int bitplanes, uint32_t datA, uint32_t datB, uint32_t datC)
{
  switch (bitplanes)
  {
    case 1:
    case 2:
    case 3: return graph_deco1[datA][1];
    case 4:
    case 5: return graph_deco1[datA][1] | graph_deco2[datB][1];
    case 6: return graph_deco1[datA][1] | graph_deco2[datB][1] | graph_deco3[datC][1];
  }
  return 0;
}

// Add modulo to the bitplane ptrs
static __inline void graphDecodeModulo(int bitplanes, uint32_t bpl_length_in_bytes)
{
  switch (bitplanes)
  {
    case 6: bpl6pt = chipsetMaskPtr(bpl6pt + bpl_length_in_bytes + bpl2mod);
    case 5: bpl5pt = chipsetMaskPtr(bpl5pt + bpl_length_in_bytes + bpl1mod);
    case 4: bpl4pt = chipsetMaskPtr(bpl4pt + bpl_length_in_bytes + bpl2mod);
    case 3: bpl3pt = chipsetMaskPtr(bpl3pt + bpl_length_in_bytes + bpl1mod);
    case 2: bpl2pt = chipsetMaskPtr(bpl2pt + bpl_length_in_bytes + bpl2mod);
    case 1: bpl1pt = chipsetMaskPtr(bpl1pt + bpl_length_in_bytes + bpl1mod);
  }
}

static void graphSetLinePointers(uint8_t **line1, uint8_t **line2)
{
  // make function compatible for no line skipping
  // Dummy lines
  *line1 = graph_line1_tmp;
  *line2 = graph_line2_tmp;
}

static __inline void graphDecodeGeneric(int bitplanes)
{
  uint32_t bpl_length_in_bytes = graph_DDF_word_count * 2;

  if (bitplanes == 0) return;
  if (bpl_length_in_bytes != 0)
  {
    uint32_t *dest_odd;
    uint32_t *dest_even;
    uint32_t *dest_tmp;
    uint32_t *end_even;
    uint8_t *pt1_tmp, *pt2_tmp, *pt3_tmp, *pt4_tmp, *pt5_tmp, *pt6_tmp;
    uint32_t dat2, dat3, dat4, dat5, dat6;
    int maxscroll;
    uint32_t temp = 0;
    uint8_t *line1;
    uint8_t *line2;

    uint32_t dat1 = dat2 = dat3 = dat4 = dat5 = dat6 = 0;

    graphSetLinePointers(&line1, &line2);

    if (_core.RegisterUtility.IsHiresEnabled()) // check if hires bit is set (bit 15 of register BPLCON0)
    {
      // high resolution
      dest_odd = (uint32_t *)(line1 + graph_DDF_start + oddhiscroll);

      // Find out how many pixels the bitplane is scrolled
      // the first pixels must then be zeroed to avoid garbage.
      maxscroll = (evenhiscroll > oddhiscroll) ? evenhiscroll : oddhiscroll;
    }
    else
    {
      dest_odd = (uint32_t *)(line1 + graph_DDF_start + oddscroll);

      // Find out how many pixels the bitplane is scrolled
      // the first pixels must then be zeroed to avoid garbage.
      maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll;
    }

    // clear edges
    while (maxscroll > 0)
    {
      line1[graph_DDF_start + temp] = 0;
      line1[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
      maxscroll--;
      temp++;
    }

    // setup loop
    uint32_t *end_odd = dest_odd + bpl_length_in_bytes * 2;

    if (bitplanes > 1)
    {
      if (_core.RegisterUtility.IsHiresEnabled()) // check if hires bit is set (bit 15 of register BPLCON0)
      {
        // high resolution
        dest_even = (uint32_t *)(line1 + graph_DDF_start + evenhiscroll);
      }
      else
      {
        // low resolution
        dest_even = (uint32_t *)(line1 + graph_DDF_start + evenscroll);
      }
      end_even = dest_even + bpl_length_in_bytes * 2;
    }

    switch (bitplanes)
    {
      case 6: pt6_tmp = memory_chip + bpl6pt;
      case 5: pt5_tmp = memory_chip + bpl5pt;
      case 4: pt4_tmp = memory_chip + bpl4pt;
      case 3: pt3_tmp = memory_chip + bpl3pt;
      case 2: pt2_tmp = memory_chip + bpl2pt;
      case 1: pt1_tmp = memory_chip + bpl1pt;
    }

    for (dest_tmp = dest_odd; dest_tmp != end_odd; dest_tmp += 2)
    {
      switch (bitplanes)
      {
        case 6:
        case 5: dat5 = *pt5_tmp++;
        case 4:
        case 3: dat3 = *pt3_tmp++;
        case 2:
        case 1: dat1 = *pt1_tmp++;
      }
      dest_tmp[0] = graphDecodeOdd1(bitplanes, dat1, dat3, dat5);
      dest_tmp[1] = graphDecodeOdd2(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2)
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 2)
      {
        switch (bitplanes)
        {
          case 6: dat6 = *pt6_tmp++;
          case 5:
          case 4: dat4 = *pt4_tmp++;
          case 3:
          case 2: dat2 = *pt2_tmp++;
        }
        dest_tmp[0] |= graphDecodeEven1(bitplanes, dat2, dat4, dat6);
        dest_tmp[1] |= graphDecodeEven2(bitplanes, dat2, dat4, dat6);
      }
    }
  }
  graphDecodeModulo(bitplanes, bpl_length_in_bytes);
}

static __inline void graphDecodeDualGeneric(int bitplanes)
{
  uint32_t bpl_length_in_bytes = graph_DDF_word_count * 2;
  if (bitplanes == 0) return;
  if (bpl_length_in_bytes != 0)
  {
    uint32_t *dest_even;
    uint32_t *dest_tmp;
    uint32_t *end_even;
    uint8_t *pt1_tmp, *pt2_tmp, *pt3_tmp, *pt4_tmp, *pt5_tmp, *pt6_tmp;
    uint32_t dat2, dat3, dat4, dat5, dat6;

    uint8_t *line1;
    uint8_t *line2;

    uint32_t dat1 = dat2 = dat3 = dat4 = dat5 = dat6 = 0;

    graphSetLinePointers(&line1, &line2);

    // clear edges
    int maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll;

    uint32_t temp = 0;
    while (maxscroll > 0)
    {
      line1[graph_DDF_start + temp] = 0;
      line2[graph_DDF_start + temp] = 0;
      line1[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
      line2[graph_DDF_start + (graph_DDF_word_count << 4) + temp] = 0;
      maxscroll--;
      temp++;
    }

    // setup loop
    uint32_t *dest_odd = (uint32_t *)(line1 + graph_DDF_start + oddscroll);
    uint32_t *end_odd = dest_odd + bpl_length_in_bytes * 2;

    if (bitplanes > 1)
    {
      // low resolution
      dest_even = (uint32_t *)(line2 + graph_DDF_start + evenscroll);
      end_even = dest_even + bpl_length_in_bytes * 2;
    }

    switch (bitplanes)
    {
      case 6: pt6_tmp = memory_chip + bpl6pt;
      case 5: pt5_tmp = memory_chip + bpl5pt;
      case 4: pt4_tmp = memory_chip + bpl4pt;
      case 3: pt3_tmp = memory_chip + bpl3pt;
      case 2: pt2_tmp = memory_chip + bpl2pt;
      case 1: pt1_tmp = memory_chip + bpl1pt;
    }

    for (dest_tmp = dest_odd; dest_tmp != end_odd; dest_tmp += 2)
    {
      switch (bitplanes)
      {
        case 6:
        case 5: dat5 = *pt5_tmp++;
        case 4:
        case 3: dat3 = *pt3_tmp++;
        case 2:
        case 1: dat1 = *pt1_tmp++;
      }
      dest_tmp[0] = graphDecodeDualOdd1(bitplanes, dat1, dat3, dat5);
      dest_tmp[1] = graphDecodeDualOdd2(bitplanes, dat1, dat3, dat5);
    }

    if (bitplanes >= 2)
    {
      for (dest_tmp = dest_even; dest_tmp != end_even; dest_tmp += 2)
      {
        switch (bitplanes)
        {
          case 6: dat6 = *pt6_tmp++;
          case 5:
          case 4: dat4 = *pt4_tmp++;
          case 3:
          case 2: dat2 = *pt2_tmp++;
        }
        dest_tmp[0] = graphDecodeDualEven1(bitplanes, dat2, dat4, dat6);
        dest_tmp[1] = graphDecodeDualEven2(bitplanes, dat2, dat4, dat6);
      }
    }
  }
  graphDecodeModulo(bitplanes, bpl_length_in_bytes);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires or lores                    */
/*===========================================================================*/

void graphDecode0()
{
  graphDecodeGeneric(0);
}

/*===========================================================================*/
/* Planar to chunky conversion, 1 bitplane hires or lores                    */
/*===========================================================================*/

void graphDecode1()
{
  graphDecodeGeneric(1);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode2()
{
  graphDecodeGeneric(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode3()
{
  graphDecodeGeneric(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 4 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode4()
{
  graphDecodeGeneric(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 5 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode5()
{
  graphDecodeGeneric(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 6 bitplanes hires or lores                   */
/*===========================================================================*/

void graphDecode6()
{
  graphDecodeGeneric(6);
}

/*===========================================================================*/
/* Planar to chunky conversion, 2 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode2Dual()
{
  graphDecodeDualGeneric(2);
}

/*===========================================================================*/
/* Planar to chunky conversion, 3 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode3Dual()
{
  graphDecodeDualGeneric(3);
}

/*===========================================================================*/
/* Planar to chunky conversion, 4 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode4Dual()
{
  graphDecodeDualGeneric(4);
}

/*===========================================================================*/
/* Planar to chunky conversion, 5 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode5Dual()
{
  graphDecodeDualGeneric(5);
}

/*===========================================================================*/
/* Planar to chunky conversion, 6 bitplanes lores, dual playfield            */
/*===========================================================================*/
void graphDecode6Dual()
{
  graphDecodeDualGeneric(6);
}

/*===========================================================================*/
/* Calculate all variables connected to the screen emulation                 */
/* First calculate the ddf variables graph_DDF_start and graph_DDF_word_count*/
/*   Check if stop is bigger than start                                      */
/*   Check if strt and stop is mod 8 aligned                                 */
/*   If NOK then set numberofwords and startpos to 0 and                     */
/*   DIWvisible vars to 128                                                  */
/*                                                                           */
/* NOTE: The following is (was? worfje) (sadly) a prime example of spaghetti code.... (PS) */
/*       Though it somehow works, so just leave it and hope it does not break*/
/*===========================================================================*/

void graphCalculateWindow()
{
  if (_core.RegisterUtility.IsHiresEnabled()) // check if Hires bit is set (bit 15 of BPLCON0)
  {
    graphCalculateWindowHires();
  }
  else
  {
    // set DDF (Display Data Fetch Start) variables
    if (ddfstop < ddfstrt)
    {
      graph_DDF_start = graph_DDF_word_count = 0;
      graph_DIW_first_visible = graph_DIW_last_visible = 256;
      return;
    }

    uint32_t ddfstop_aligned = ddfstop & 0x07;
    uint32_t ddfstrt_aligned = ddfstrt & 0x07;

    if (ddfstop >= ddfstrt)
    {
      graph_DDF_word_count = ((ddfstop - ddfstrt) >> 3) + 1;
    }
    else
    {
      graph_DDF_word_count = ((0xd8 - ddfstrt) >> 3) + 1;
      ddfstop_aligned = 0;
    }

    if (ddfstop_aligned != ddfstrt_aligned)
    {
      graph_DDF_word_count++;
    }

    graph_DDF_start = ((ddfstrt << 1) + 0x11); // 0x11 = 17 pixels = 8.5 bus cycles

    // set DIW (Display Window Start) variables (only used with drawing routines)

    if (diwxleft >= diwxright)
    {
      graph_DDF_start = graph_DDF_word_count = 0;
      graph_DIW_first_visible = graph_DIW_last_visible = 256;
    }

    graph_DIW_first_visible = drawGetInternalClip().left;
    if (diwxleft < graph_DDF_start)
    {
      if (graph_DDF_start > graph_DIW_first_visible)
      {
        graph_DIW_first_visible = graph_DDF_start;
      }
    }
    else
    {
      if (diwxleft > graph_DIW_first_visible)
      {
        graph_DIW_first_visible = diwxleft;
      }
    }

    uint32_t last_position_in_line = (graph_DDF_word_count << 4) + graph_DDF_start;
    if (oddscroll > evenscroll)
    {
      last_position_in_line += oddscroll;
    }
    else
    {
      last_position_in_line += evenscroll;
    }

    graph_DIW_last_visible = drawGetInternalClip().right;
    if (last_position_in_line < diwxright)
    {
      if (last_position_in_line < graph_DIW_last_visible)
      {
        graph_DIW_last_visible = last_position_in_line;
      }
    }
    else
    {
      if (diwxright < graph_DIW_last_visible)
      {
        graph_DIW_last_visible = diwxright;
      }
    }
  }
}

void graphCalculateWindowHires()
{
  if (ddfstrt > ddfstop)
  {
    graph_DDF_start = graph_DDF_word_count = 0;
    graph_DIW_first_visible = graph_DIW_last_visible = 256;
    return;
  }

  if (ddfstop >= ddfstrt)
  {
    graph_DDF_word_count = (((ddfstop - ddfstrt) + 15) >> 2) & 0x0FFFFFFFE;
  }
  else
  {
    graph_DDF_word_count = (((0xd8 - ddfstrt) + 15) >> 2) & 0x0FFFFFFFE;
  }

  graph_DDF_start = (ddfstrt << 2) + 18;

  // DDF variables done, now check DIW variables

  if ((diwxleft << 1) >= (diwxright << 1))
  {
    graph_DDF_start = graph_DDF_word_count = 0;
    graph_DIW_first_visible = graph_DIW_last_visible = 256;
  }
  uint32_t clip_left = drawGetInternalClip().left << 1;
  if ((diwxleft << 1) < graph_DDF_start)
  {
    if (graph_DDF_start > clip_left)
    {
      graph_DIW_first_visible = graph_DDF_start;
    }
    else
    {
      graph_DIW_first_visible = clip_left;
    }
  }
  else
  {
    if ((diwxleft << 1) > clip_left)
    {
      graph_DIW_first_visible = diwxleft << 1;
    }
    else
    {
      graph_DIW_first_visible = clip_left;
    }
  }

  uint32_t last_position_in_line = graph_DDF_start + (graph_DDF_word_count << 4);
  if (oddhiscroll > evenhiscroll)
  {
    last_position_in_line += oddhiscroll;
  }
  else
  {
    last_position_in_line += evenhiscroll;
  }

  uint32_t clip_right = drawGetInternalClip().right << 1;
  if (last_position_in_line < (diwxright << 1))
  {
    if (last_position_in_line < clip_right)
    {
      graph_DIW_last_visible = last_position_in_line;
    }
    else
    {
      graph_DIW_last_visible = clip_right;
    }
  }
  else
  {
    if ((diwxright << 1) < clip_right)
    {
      graph_DIW_last_visible = diwxright << 1;
    }
    else
    {
      graph_DIW_last_visible = clip_right;
    }
  }
}

void graphPlayfieldOnOff()
{
  uint32_t currentY = _core.Timekeeper->GetAgnusLine();
  if (graph_playfield_on != 0)
  {
    // Playfield on, check if top has moved below graph_raster_y
    // Playfield on, check if playfield is turned off at this line
    if (currentY == diwybottom)
    {
      graph_playfield_on = 0;
    }
  }
  else
  {
    // Playfield off, Check if playfield is turned on at this line
    if (currentY == diwytop)
    {
      // Don't turn on when top > bottom
      if (diwytop < diwybottom)
      {
        graph_playfield_on = 1;
      }
    }
  }
  // OK, here the state of the playfield is taken care of
}

void graphDecodeNOP()
{
  switch (_core.RegisterUtility.GetEnabledBitplaneCount())
  {
    case 0: break;
    case 6: bpl6pt = chipsetMaskPtr(bpl6pt + (graph_DDF_word_count * 2) + bpl2mod);
    case 5: bpl5pt = chipsetMaskPtr(bpl5pt + (graph_DDF_word_count * 2) + bpl1mod);
    case 4: bpl4pt = chipsetMaskPtr(bpl4pt + (graph_DDF_word_count * 2) + bpl2mod);
    case 3: bpl3pt = chipsetMaskPtr(bpl3pt + (graph_DDF_word_count * 2) + bpl1mod);
    case 2: bpl2pt = chipsetMaskPtr(bpl2pt + (graph_DDF_word_count * 2) + bpl2mod);
    case 1: bpl1pt = chipsetMaskPtr(bpl1pt + (graph_DDF_word_count * 2) + bpl1mod); break;
  }
}

/*-------------------------------------------------------------------------------
/* Copy color block to line description
/* [4 + esp] - linedesc struct
/*-------------------------------------------------------------------------------*/

void graphLinedescColors(graph_line *current_graph_line)
{
  uint32_t color;

  color = 0;
  while (color < 64)
  {
    current_graph_line->colors[color] = graph_color_shadow[color];
    color++;
  }
}

/*-------------------------------------------------------------------------------
/* Sets line routines for this line
/* [4 + esp] - linedesc struct
/*-------------------------------------------------------------------------------*/

void graphLinedescRoutines(graph_line *current_graph_line)
{
  /*==============================================================
  /* Set drawing routines
  /*==============================================================*/
  current_graph_line->draw_line_routine = draw_line_routine;
  current_graph_line->draw_line_BPL_res_routine = draw_line_BPL_res_routine;
}

/*-------------------------------------------------------------------------------
/* Sets line geometry data in line description
/* [4 + esp] - linedesc struct
/*-------------------------------------------------------------------------------*/

void graphLinedescGeometry(graph_line *current_graph_line)
{
  uint32_t local_graph_DIW_first_visible = graph_DIW_first_visible;
  int32_t local_graph_DIW_last_visible = (int32_t)graph_DIW_last_visible;
  uint32_t local_graph_DDF_start = graph_DDF_start;
  uint32_t local_draw_left = drawGetInternalClip().left;
  uint32_t local_draw_right = drawGetInternalClip().right;
  uint32_t shift = 0;

  /*===========================================================*/
  /* Calculate first and last visible DIW and DIW pixel count  */
  /*===========================================================*/

  if (_core.RegisterUtility.IsHiresEnabled())
  {
    // bit 15, HIRES is set
    local_graph_DIW_first_visible >>= 1;
    local_graph_DIW_last_visible >>= 1;
    local_graph_DDF_start >>= 1;
    shift = 1;
  }
  if (local_graph_DIW_first_visible < local_draw_left)
  {
    local_graph_DIW_first_visible = local_draw_left;
  }
  if (local_graph_DIW_last_visible > (int32_t)local_draw_right)
  {
    local_graph_DIW_last_visible = (int32_t)local_draw_right;
  }
  local_graph_DIW_last_visible -= local_graph_DIW_first_visible;
  if (local_graph_DIW_last_visible < 0)
  {
    local_graph_DIW_last_visible = 0;
  }
  local_graph_DIW_first_visible <<= shift;
  local_graph_DIW_last_visible <<= shift;
  current_graph_line->DIW_first_draw = local_graph_DIW_first_visible;
  current_graph_line->DIW_pixel_count = local_graph_DIW_last_visible;
  current_graph_line->DDF_start = local_graph_DDF_start;
  local_graph_DIW_first_visible >>= shift;
  local_graph_DIW_last_visible >>= shift;

  /*=========================================*/
  /* Calculate BG front and back pad count   */
  /*=========================================*/

  local_draw_right -= local_graph_DIW_first_visible;
  local_graph_DIW_first_visible -= local_draw_left;
  local_draw_right -= local_graph_DIW_last_visible;
  current_graph_line->BG_pad_front = local_graph_DIW_first_visible;
  current_graph_line->BG_pad_back = local_draw_right;

  /*==========================================================*/
  /* Need to remember playfield priorities to sort dual pf    */
  /*==========================================================*/

  current_graph_line->bplcon2 = _core.Registers.BplCon2;
}

/*-------------------------------------------------------------------------------
/* Smart sets line routines for this line
/* Return TRUE if routines have changed
/*-------------------------------------------------------------------------------*/
BOOLE graphLinedescRoutinesSmart(graph_line *current_graph_line)
{
  /*==============================================================*/
  /* Set drawing routines                                         */
  /*==============================================================*/

  BOOLE result = FALSE;
  if (current_graph_line->draw_line_routine != draw_line_routine)
  {
    result = TRUE;
  }
  current_graph_line->draw_line_routine = draw_line_routine;

  if (current_graph_line->draw_line_BPL_res_routine != draw_line_BPL_res_routine)
  {
    result = TRUE;
  }
  current_graph_line->draw_line_BPL_res_routine = draw_line_BPL_res_routine;
  return result;
}

/*-------------------------------------------------------------------------------
/* Sets line geometry data in line description
/* Return TRUE if geometry has changed
/*-------------------------------------------------------------------------------*/
BOOLE graphLinedescGeometrySmart(graph_line *current_graph_line)
{
  uint32_t local_graph_DIW_first_visible = graph_DIW_first_visible;
  int32_t local_graph_DIW_last_visible = (int32_t)graph_DIW_last_visible;
  uint32_t local_graph_DDF_start = graph_DDF_start;
  uint32_t local_draw_left = drawGetInternalClip().left;
  uint32_t local_draw_right = drawGetInternalClip().right;
  uint32_t shift = 0;
  BOOLE line_desc_changed = FALSE;

  /*===========================================================*/
  /* Calculate first and last visible DIW and DIW pixel count  */
  /*===========================================================*/

  if (_core.RegisterUtility.IsHiresEnabled())
  {
    // bit 15, HIRES is set
    local_graph_DIW_first_visible >>= 1;
    local_graph_DIW_last_visible >>= 1;
    local_graph_DDF_start >>= 1;
    shift = 1;
  }
  if (local_graph_DIW_first_visible < local_draw_left)
  {
    local_graph_DIW_first_visible = local_draw_left;
  }
  if (local_graph_DIW_last_visible > (int32_t)local_draw_right)
  {
    local_graph_DIW_last_visible = (int32_t)local_draw_right;
  }
  local_graph_DIW_last_visible -= local_graph_DIW_first_visible;
  if (local_graph_DIW_last_visible < 0)
  {
    local_graph_DIW_last_visible = 0;
  }

  local_graph_DIW_first_visible <<= shift;
  local_graph_DIW_last_visible <<= shift;
  if (current_graph_line->DIW_first_draw != local_graph_DIW_first_visible)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->DIW_first_draw = local_graph_DIW_first_visible;
  if (current_graph_line->DIW_pixel_count != local_graph_DIW_last_visible)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->DIW_pixel_count = local_graph_DIW_last_visible;
  if (current_graph_line->DDF_start != local_graph_DDF_start)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->DDF_start = local_graph_DDF_start;
  local_graph_DIW_first_visible >>= shift;
  local_graph_DIW_last_visible >>= shift;

  /*=========================================*/
  /* Calculate BG front and back pad count   */
  /*=========================================*/

  local_draw_right -= local_graph_DIW_first_visible;
  local_graph_DIW_first_visible -= local_draw_left;
  local_draw_right -= local_graph_DIW_last_visible;
  if (current_graph_line->BG_pad_front != local_graph_DIW_first_visible)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->BG_pad_front = local_graph_DIW_first_visible;
  if (current_graph_line->BG_pad_back != local_draw_right)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->BG_pad_back = local_draw_right;

  /*==========================================================*/
  /* Need to remember playfield priorities to sort dual pf    */
  /*==========================================================*/

  if (current_graph_line->bplcon2 != _core.Registers.BplCon2)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->bplcon2 = _core.Registers.BplCon2;
  return line_desc_changed;
}

/*-------------------------------------------------------------------------------
/* Smart copy color block to line description
/* Return TRUE if colors have changed
/*-------------------------------------------------------------------------------*/

BOOLE graphLinedescColorsSmart(graph_line *current_graph_line)
{
  BOOLE result = FALSE;

  // check full brightness colors
  for (uint32_t i = 0; i < 32; i++)
  {
    if (graph_color_shadow[i] != current_graph_line->colors[i])
    {
      current_graph_line->colors[i] = graph_color_shadow[i];
      current_graph_line->colors[i + 32] = graph_color_shadow[i + 32];
      result = TRUE;
    }
  }
  return result;
}

/*-------------------------------------------------------------------------------
/* Copy remainder of the data, data changed, no need to compare anymore.
/* Return TRUE = not equal
/*-------------------------------------------------------------------------------*/

static BOOLE graphCompareCopyRest(uint32_t first_pixel, int32_t pixel_count, uint8_t *dest_line, uint8_t *source_line)
{
  // line has changed, copy the rest
  while ((first_pixel & 0x3) != 0)
  {
    dest_line[first_pixel] = source_line[first_pixel];
    first_pixel++;
    pixel_count--;
    if (pixel_count == 0)
    {
      return TRUE;
    }
  }

  while (pixel_count >= 4)
  {
    *((uint32_t *)(dest_line + first_pixel)) = *((uint32_t *)(source_line + first_pixel));
    first_pixel += 4;
    pixel_count -= 4;
  }

  while (pixel_count > 0)
  {
    dest_line[first_pixel] = source_line[first_pixel];
    first_pixel++;
    pixel_count--;
  }
  return TRUE;
}

/*-------------------------------------------------------------------------------
/* Copy data and compare
/* Return TRUE = not equal FALSE = equal
/*-------------------------------------------------------------------------------*/

static BOOLE graphCompareCopy(uint32_t first_pixel, int32_t pixel_count, uint8_t *dest_line, uint8_t *source_line)
{
  BOOLE result = FALSE;

  if (pixel_count > 0)
  {
    // align to 4-byte boundary
    while ((first_pixel & 0x3) != 0)
    {
      if (dest_line[first_pixel] == source_line[first_pixel])
      {
        first_pixel++;
        pixel_count--;
        if (pixel_count == 0)
        {
          return FALSE;
        }
      }
      else
      {
        // line has changed, copy the rest
        return graphCompareCopyRest(first_pixel, pixel_count, dest_line, source_line);
      }
    }

    // compare dword aligned values
    while (pixel_count >= 4)
    {
      if (*((uint32_t *)(source_line + first_pixel)) == *((uint32_t *)(dest_line + first_pixel)))
      {
        first_pixel += 4;
        pixel_count -= 4;
      }
      else
      {
        // line has changed, copy the rest
        return graphCompareCopyRest(first_pixel, pixel_count, dest_line, source_line);
      }
    }

    // compare last couple of bytes
    while (pixel_count > 0)
    {
      if (source_line[first_pixel] == dest_line[first_pixel])
      {
        first_pixel++;
        pixel_count--;
      }
      else
      {
        result = TRUE;
        dest_line[first_pixel] = source_line[first_pixel];
        first_pixel++;
        pixel_count--;
      }
    }
  }
  return result;
}

BOOLE graphLinedescSetBackgroundLine(graph_line *current_graph_line)
{
  if (graph_color_shadow[0] == current_graph_line->colors[0])
  {
    if (current_graph_line->linetype == graph_linetypes::GRAPH_LINE_SKIP)
    {
      // Already skipping this line and no changes
      return FALSE;
    }
    if (current_graph_line->linetype == graph_linetypes::GRAPH_LINE_BG)
    {
      /*==================================================================*/
      /* This line was a "background" line during the last drawing of     */
      /* this frame buffer,                                               */
      /* and it has the same color as last time.                          */
      /* We might be able to skip drawing it, we need to check the        */
      /* flag that says it has been drawn in all of our buffers.          */
      /*==================================================================*/
      if (current_graph_line->frames_left_until_BG_skip == 0)
      {
        // No changes since drawing it last time, change state to skip
        current_graph_line->linetype = graph_linetypes::GRAPH_LINE_SKIP;
        return FALSE;
      }
      else
      {
        // Still need to draw the line in some buffers
        current_graph_line->frames_left_until_BG_skip--;
        return TRUE;
      }
    }
  }

  /*==================================================================*/
  /* This background line is different from the one in the buffer     */
  /*==================================================================*/
  current_graph_line->linetype = graph_linetypes::GRAPH_LINE_BG;
  current_graph_line->colors[0] = graph_color_shadow[0];
  current_graph_line->frames_left_until_BG_skip = drawGetBufferCount() - 1;
  graphLinedescRoutines(current_graph_line);
  return TRUE;
}

BOOLE graphLinedescSetBitplaneLine(graph_line *current_graph_line)
{
  /*===========================================*/
  /* This is a bitplane line                   */
  /*===========================================*/
  if (current_graph_line->linetype != graph_linetypes::GRAPH_LINE_BPL)
  {
    if (current_graph_line->linetype != graph_linetypes::GRAPH_LINE_BPL_SKIP)
    {
      current_graph_line->linetype = graph_linetypes::GRAPH_LINE_BPL;
      graphLinedescColors(current_graph_line);
      graphLinedescGeometry(current_graph_line);
      graphLinedescRoutines(current_graph_line);
      return TRUE;
    }
  }
  BOOLE line_desc_changed = graphLinedescColorsSmart(current_graph_line);
  line_desc_changed |= graphLinedescGeometrySmart(current_graph_line);
  line_desc_changed |= graphLinedescRoutinesSmart(current_graph_line);
  return line_desc_changed;
}

/*-------------------------------------------------------------------------------
/* Smart makes a description of this line
/* Return TRUE if linedesc has changed
/*-------------------------------------------------------------------------------*/

BOOLE graphLinedescMakeSmart(graph_line *current_graph_line)
{
  /*==========================================================*/
  /* Is this a bitplane or background line?                   */
  /*==========================================================*/
  if (draw_line_BG_routine != draw_line_routine)
  {
    return graphLinedescSetBitplaneLine(current_graph_line);
  }

  return graphLinedescSetBackgroundLine(current_graph_line);
}

/*===========================================================================*/
/* Smart compose the visible layout of the line                              */
/*===========================================================================*/

void graphComposeLineOutputSmart(graph_line *current_graph_line)
{
  // remember the basic properties of the line
  BOOLE line_desc_changed = graphLinedescMakeSmart(current_graph_line);

  // check if we need to decode bitplane data
  if (draw_line_BG_routine != draw_line_routine)
  {
    // do the planar to chunky conversion
    // stuff the data into a temporary array
    // then copy it and compare
    graph_decode_line_ptr();

    // compare line data to old data
    line_desc_changed |= graphCompareCopy(current_graph_line->DIW_first_draw, (int32_t)(current_graph_line->DIW_pixel_count), current_graph_line->line1, graph_line1_tmp);

    // if the line is dual playfield, compare second playfield too
    if (_core.RegisterUtility.IsDualPlayfieldEnabled())
    {
      line_desc_changed |= graphCompareCopy(current_graph_line->DIW_first_draw, (int32_t)(current_graph_line->DIW_pixel_count), current_graph_line->line2, graph_line2_tmp);
    }

    if (current_graph_line->has_ham_sprites_online)
    {
      // Compare will not detect old HAM sprites by looking at pixel data etc. Always mark line as dirty.
      line_desc_changed = TRUE;
      current_graph_line->has_ham_sprites_online = false;
    }

    // add sprites to the line image
    if (_core.LineExactSprites->HasSpritesOnLine())
    {
      line_desc_changed = TRUE;
      _core.LineExactSprites->Merge(current_graph_line);
    }

    // final test for line skip
    if (line_desc_changed)
    {
      current_graph_line->linetype = graph_linetypes::GRAPH_LINE_BPL;
    }
    else
    {
      current_graph_line->linetype = graph_linetypes::GRAPH_LINE_BPL_SKIP;
    }
  }
  else
  {
    current_graph_line->has_ham_sprites_online = false;
  }
}

/*===========================================================================*/
/* Initializes the Planar 2 Chunky translation tables                        */
/*===========================================================================*/

void graphP2CTablesInit()
{
  uint32_t d[2];

  for (uint32_t i = 0; i < 256; i++)
  {
    d[0] = d[1] = 0;
    for (uint32_t j = 0; j < 4; j++)
    {
      d[0] |= ((i & (0x80 >> j)) >> (4 + 3 - j)) << (j * 8);
      d[1] |= ((i & (0x8 >> j)) >> (3 - j)) << (j * 8);
    }
    for (uint32_t j = 0; j < 2; j++)
    {
      graph_deco1[i][j] = d[j] << 2;
      graph_deco2[i][j] = d[j] << 3;
      graph_deco3[i][j] = d[j] << 4;
      graph_deco4[i][j] = d[j] << 5;
      graph_deco5[i][j] = d[j] << 6;
      graph_deco6[i][j] = d[j] << 7;
    }
  }
}

/*===========================================================================*/
/* Decode tables                                                             */
/* Called from the draw module                                               */
/*===========================================================================*/

static void graphP2CFunctionsInit()
{
  graph_decode_line_tab[0] = graphDecode0;
  graph_decode_line_tab[1] = graphDecode1;
  graph_decode_line_tab[2] = graphDecode2;
  graph_decode_line_tab[3] = graphDecode3;
  graph_decode_line_tab[4] = graphDecode4;
  graph_decode_line_tab[5] = graphDecode5;
  graph_decode_line_tab[6] = graphDecode6;
  graph_decode_line_tab[7] = graphDecode0;
  graph_decode_line_tab[8] = graphDecode0;
  graph_decode_line_tab[9] = graphDecode1;
  graph_decode_line_tab[10] = graphDecode2;
  graph_decode_line_tab[11] = graphDecode3;
  graph_decode_line_tab[12] = graphDecode4;
  graph_decode_line_tab[13] = graphDecode0;
  graph_decode_line_tab[14] = graphDecode0;
  graph_decode_line_tab[15] = graphDecode0;
  graph_decode_line_dual_tab[0] = graphDecode0;
  graph_decode_line_dual_tab[1] = graphDecode1;
  graph_decode_line_dual_tab[2] = graphDecode2Dual;
  graph_decode_line_dual_tab[3] = graphDecode3Dual;
  graph_decode_line_dual_tab[4] = graphDecode4Dual;
  graph_decode_line_dual_tab[5] = graphDecode5Dual;
  graph_decode_line_dual_tab[6] = graphDecode6Dual;
  graph_decode_line_dual_tab[7] = graphDecode0;
  graph_decode_line_dual_tab[8] = graphDecode0;
  graph_decode_line_dual_tab[9] = graphDecode1;
  graph_decode_line_dual_tab[10] = graphDecode2Dual;
  graph_decode_line_dual_tab[11] = graphDecode3Dual;
  graph_decode_line_dual_tab[12] = graphDecode4Dual;
  graph_decode_line_dual_tab[13] = graphDecode0;
  graph_decode_line_dual_tab[14] = graphDecode0;
  graph_decode_line_dual_tab[15] = graphDecode0;
  graph_decode_line_ptr = graphDecode0;
}

/*===========================================================================*/
/* End of line handler for graphics                                          */
/*===========================================================================*/

void graphEndOfLine()
{
  if (drawGetGraphicsEmulationMode() == GRAPHICSEMULATIONMODE::GRAPHICSEMULATIONMODE_CYCLEEXACT)
  {
    GraphicsContext.Commit(_core.Timekeeper->GetAgnusLine(), _core.Timekeeper->GetAgnusLineCycle());
    return;
  }

  // skip this frame?
  if (draw_frame_skip == 0)
  {
    uint32_t currentY = _core.Timekeeper->GetAgnusLine();
    // update diw state
    graphPlayfieldOnOff();

    // skip lines before line $12
    if (currentY >= 0x12)
    {
      // make pointer to linedesc for this line
      graph_line *current_graph_line = graphGetLineDesc(draw_buffer_draw, currentY);

      // decode sprites if DMA is enabled and raster is after line $18
      if ((dmacon & 0x20) == 0x20)
      {
        if (currentY >= 0x18)
        {
          _core.LineExactSprites->DMASpriteHandler();
          _core.LineExactSprites->ProcessActionList();
        }
      }

      // sprites decoded, sprites_onlineflag is set if there are any
      // update pointer to drawing routine
      drawUpdateDrawmode();

      // check if we are clipped
      if ((currentY >= drawGetInternalClip().top) || (currentY < drawGetInternalClip().bottom))
      {
        // visible line, either background or bitplanes
        graphComposeLineOutputSmart(current_graph_line);
      }
      else
      {
        // do nop decoding, no screen output needed
        if (draw_line_BG_routine != draw_line_routine)
        {
          graphDecodeNOP();
        }
      }

      // if diw state changing from background to bitplane output,
      // set new drawing routine pointer

      if (draw_switch_bg_to_bpl != 0)
      {
        draw_line_BPL_manage_routine = draw_line_routine;
        draw_switch_bg_to_bpl = 0;
      }

      if (currentY == (_core.CurrentFrameParameters->LinesInFrame - 1))
      {
        // In the case when the display has more lines than the frame (AF or short frames)
        // this routine pads the remaining lines with background color
        for (uint32_t y = currentY + 1; y < drawGetInternalClip().bottom; ++y)
        {
          graph_line *graph_line_y = graphGetLineDesc(draw_buffer_draw, y);
          graphLinedescSetBackgroundLine(graph_line_y);
        }
      }
    }
  }
}

/*===========================================================================*/
/* Called on emulation end of frame                                          */
/*===========================================================================*/

void graphEndOfFrame()
{
  graph_playfield_on = FALSE;
  if (graph_buffer_lost == TRUE)
  {
    graphLineDescClear();
    graph_buffer_lost = FALSE;
  }
}

/*===========================================================================*/
/* Called on emulation hard reset                                            */
/*===========================================================================*/

void graphHardReset()
{
  graphIORegistersClear();
  graphLineDescClear();
}

/*===========================================================================*/
/* Called on emulation start                                                 */
/*===========================================================================*/

void graphEmulationStart()
{
  graph_buffer_lost = FALSE;
  graphLineDescClear();
  graphIOHandlersInstall();
}

/*===========================================================================*/
/* Called on emulation stop                                                  */
/*===========================================================================*/

void graphEmulationStop()
{
}

/*===========================================================================*/
/* Initializes the graphics module                                           */
/*===========================================================================*/

void graphStartup()
{
  graphP2CTablesInit();
  graphP2CFunctionsInit();
  graphLineDescClear();
  graphIORegistersClear();
}

/*===========================================================================*/
/* Release resources taken by the graphics module                            */
/*===========================================================================*/

void graphShutdown()
{
}

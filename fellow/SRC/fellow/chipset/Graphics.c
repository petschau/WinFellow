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

#include "fellow/api/defs.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/memory/Memory.h"
#include "CopperCommon.h"
#include "fellow/chipset/Blitter.h"
#include "fellow/chipset/Graphics.h"
#include "fellow/scheduler/Scheduler.h"
#include "SOUND.H"
#include "LineExactSprites.h"
#include "fellow/cpu/CpuIntegration.h"
#include "draw_interlace_control.h"
#include "draw_pixelrenderers.h"

#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/Planar2ChunkyDecoder.h"

//=====================================================================
// flag that handles loss of surface content due to DirectX malfunction
//=====================================================================

bool graph_buffer_lost;

//============================================
// Lookup-tables for planar to chunky routines
//============================================

planar2chunkyroutine graph_decode_line_ptr;
planar2chunkyroutine graph_decode_line_tab[16];
planar2chunkyroutine graph_decode_line_dual_tab[16];

UBY graph_line1_tmp[1024];
UBY graph_line2_tmp[1024];

//====================================
// Line description registers and data
//====================================

ULO graph_DDF_start;
ULO graph_DDF_word_count;
ULO graph_DIW_first_visible;
ULO graph_DIW_last_visible;
bool graph_playfield_on;

//=============
// IO-registers
//=============

ULO evenscroll, evenhiscroll, oddscroll, oddhiscroll;
ULO diwxleft, diwxright, diwytop, diwybottom;
ULO dmaconr, dmacon;

//=======================================================
// Framebuffer data about each line, max triple buffering
//=======================================================

graph_line graph_frame[3][628];

//======================================================================
// Clear the line descriptions
// This function is called on emulation start and from the debugger when
// the emulator is stopped mid-line. It must create a valid dummy array
//======================================================================

void graphLineDescClear()
{
  memset(graph_frame, 0, sizeof(graph_line) * 3 * 628);
  for (int frame = 0; frame < 3; frame++)
  {
    for (int line = 0; line < 628; line++)
    {
      graph_frame[frame][line].linetype = graph_linetypes::GRAPH_LINE_BG;
      graph_frame[frame][line].draw_line_routine = draw_line_BG_routine;
      graph_frame[frame][line].colors[0] = 0;
      graph_frame[frame][line].frames_left_until_BG_skip = Draw.GetBufferCount(); // ie. one more than normal to draw once in each buffer
      graph_frame[frame][line].sprite_ham_slot = 0xffffffff;
      graph_frame[frame][line].has_ham_sprites_online = false;
    }
  }
}

graph_line *graphGetLineDesc(int buffer_no, unsigned int currentY)
{
  unsigned int line_index = currentY * 2;

  if (drawGetUseInterlacedRendering() && !drawGetFrameIsLong())
  {
    line_index += 1;
  }
  return &graph_frame[buffer_no][line_index];
}

//========================
// Clear all register data
//========================

static void graphIORegistersClear()
{
  graph_playfield_on = false;
  graph_DDF_start = 0;
  graph_DDF_word_count = 0;

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
  dmaconr = 0;
  dmacon = 0;
}

//==================================
// Graphics Register Access routines
//==================================

//==========================================
// DMACONR - $dff002 Read
//
// return dmaconr | ((!((WOR)bltzero))<<13);
//==========================================

UWO rdmaconr(ULO address)
{
  if (blitterGetZeroFlag())
  {
    return (UWO)(dmaconr | 0x00002000);
  }
  return (UWO)dmaconr;
}

void graphNotifyDIWStrtChanged()
{
  diwytop = BitplaneUtility::GetDIWYStart();
  if (diwxright == 472)
  {
    diwxleft = 88;
  }
  else
  {
    diwxleft = BitplaneUtility::GetDIWXStart() >> 2;
  }

  graphCalculateWindow();
  graphPlayfieldOnOff();
}

void graphNotifyDIWStopChanged()
{
  diwybottom = BitplaneUtility::GetDIWYStop();
  diwxright = BitplaneUtility::GetDIWXStop() >> 2;

  if (diwxright >= 458)
  {
    diwxleft = 88;
    diwxright = 472;
  }

  graphCalculateWindow();
  graphPlayfieldOnOff();
}

void graphNotifyDDFStrtChanged()
{
  graphNotifyBplCon1Changed();
}

void graphNotifyDDFStopChanged()
{
  graphCalculateWindow();
}

//=======================================
// DMACON - $dff096 Write
//
// $dff096  - Read from $dff002
// dmacon - zero if master dma bit is off
// dmaconr - is always correct
//=======================================

void wdmacon(UWO data, ULO address)
{
  ULO prev_dmacon = dmacon;

  // check SET/CLR bit is 1 or 0
  if ((data & 0x8000) != 0x0)
  {
    // SET/CLR bit is 1 (bits set to 1 will set bits)
    ULO local_data = data & 0x7ff; // zero bits 15 to 11 (readonly bits and set/clr bit)
    // test if BLTPRI got turned on (blitter DMA priority)
    if ((local_data & 0x0400) != 0x0)
    {
      // BLTPRI bit is on now
      if ((dmaconr & 0x0400) == 0x0)
      {
        // BLTPRI was turned off before and therefor
        // BLTPRI got turned on, stop CPU until a blit is
        // finished if this is a blit that uses all cycles
        if (blitterEvent.IsEnabled())
        {
          if (blitterGetFreeCycles() == 0)
          {
            // below delays CPU additionally cycles
            cpuIntegrationSetChipCycles(cpuIntegrationGetChipCycles() + (blitterEvent.cycle - scheduler.GetFrameCycle()));
          }
        }
      }
    }

    dmaconr |= local_data;
    if ((dmaconr & 0x0200) == 0x0)
    {
      dmacon = 0;
    }
    else
    {
      dmacon = dmaconr;
    }

    // enable audio channel X ?
    for (ULO i = 0; i < 4; i++)
    {
      if (((dmacon & (1 << i))) != 0x0)
      {
        // audio channel 0 DMA enable bit is set now
        if ((prev_dmacon & (1 << i)) == 0x0)
        {
          // audio channel 0 DMA enable bit was clear previously
          soundChannelEnable(i);
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
  }
  else
  {
    // SET/CLR bit is 0 (bits set to 1 will clear bits)
    dmaconr = (~(data & 0x07ff)) & dmaconr;
    if ((dmaconr & 0x0200) == 0x0)
    {
      dmacon = 0;
    }
    else
    {
      dmacon = dmaconr;
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
    for (ULO i = 0; i < 4; i++)
    {
      if ((dmacon & (1 << i)) == 0x0)
      {
        if ((prev_dmacon & (1 << i)) != 0x0)
        {
          soundChannelKill(i);
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
  }

  if ((prev_dmacon & 0x80) != (dmacon & 0x80))
  {
    copperNotifyDMAEnableChanged((dmacon & 0x80) == 0x80);
  }
}

void graphNotifyBplCon0Changed()
{
  ULO local_data = (bitplane_registers.bplcon0 >> 12) & 0x0f;

  // check if DBLPF bit is set
  if ((bitplane_registers.bplcon0 & 0x0400) != 0)
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
  if ((bitplane_registers.bplcon0 & 0x0800) != 0)
  {
    // hold-and-modify mode
    draw_line_BPL_res_routine = draw_line_HAM_lores_routine;
  }
  else
  {
    // check if DBLPF bit is set
    if ((bitplane_registers.bplcon0 & 0x0400) != 0)
    {
      // check if HIRES is set
      if ((bitplane_registers.bplcon0 & 0x8000) != 0)
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
      if ((bitplane_registers.bplcon0 & 0x8000) != 0)
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

// BPLCON1 and DDFSTRT affect each other
// when ddfstrt is (mod 8)+4, shift order is 8-f,0-7 (lores) (Example: New Zealand Story)
// when ddfstrt is (mod 8)+2, shift order is 4-7,0-3 (hires)
void graphNotifyBplCon1Changed()
{
  ULO ddfstrt_limit = ((bitplane_registers.ddfstrt & 0xfc) < 0x18) ? 0x18 : (bitplane_registers.ddfstrt & 0xfc);

  // check for reverse shift order
  if ((ddfstrt_limit & 0x04) != 0)
  {
    oddscroll = ((bitplane_registers.bplcon1 & 0x0f) + 8) & 0x0f;
  }
  else
  {
    oddscroll = bitplane_registers.bplcon1 & 0x0f;
  }

  // check for ?
  if ((ddfstrt_limit & 0x02) != 0)
  {
    oddhiscroll = (((oddscroll & 0x07) + 4) & 0x07) << 1;
  }
  else
  {
    oddhiscroll = (oddscroll & 0x07) << 1;
  }

  // check for reverse shift order
  if ((ddfstrt_limit & 0x04) != 0)
  {
    evenscroll = (((bitplane_registers.bplcon1 & 0xf0) >> 4) + 8) & 0x0f;
  }
  else
  {
    evenscroll = (bitplane_registers.bplcon1 & 0xf0) >> 4;
  }

  // check for ?
  if ((ddfstrt_limit & 0x02) != 0)
  {
    evenhiscroll = (((evenscroll & 0x07) + 4) & 0x07) << 1;
  }
  else
  {
    evenhiscroll = (evenscroll & 0x07) << 1;
  }

  graphCalculateWindow();
}

//============================================
// Registers the graphics IO register handlers
//============================================

void graphIOHandlersInstall()
{
  memorySetIoReadStub(0x002, rdmaconr);
  memorySetIoWriteStub(0x096, wdmacon);

  // TODO: Move this somewhere else
  bitplane_registers.InstallIOHandlers();
}

static void graphSetLinePointers(UBY **line1, UBY **line2)
{
  // make function compatible for no line skipping
  // Dummy lines
  *line1 = graph_line1_tmp;
  *line2 = graph_line2_tmp;
}

static void graphDecodeGeneric(int bitplanes)
{
  ULO bpl_length_in_bytes = graph_DDF_word_count * 2;

  if (bitplanes == 0) return;
  if (bpl_length_in_bytes != 0)
  {
    ULO *dest_odd;
    ULO *dest_even;
    ULO *dest_tmp;
    ULO *end_even;
    UBY *pt1_tmp, *pt2_tmp, *pt3_tmp, *pt4_tmp, *pt5_tmp, *pt6_tmp;
    ULO dat1, dat2, dat3, dat4, dat5, dat6;
    int maxscroll;
    ULO temp = 0;
    UBY *line1;
    UBY *line2;

    dat1 = dat2 = dat3 = dat4 = dat5 = dat6 = 0;

    graphSetLinePointers(&line1, &line2);

    if ((bitplane_registers.bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
    {
      // high resolution
      dest_odd = (ULO *)(line1 + graph_DDF_start + oddhiscroll);

      // Find out how many pixels the bitplane is scrolled
      // the first pixels must then be zeroed to avoid garbage.
      maxscroll = (evenhiscroll > oddhiscroll) ? evenhiscroll : oddhiscroll;
    }
    else
    {
      dest_odd = (ULO *)(line1 + graph_DDF_start + oddscroll);

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
    ULO *end_odd = dest_odd + bpl_length_in_bytes * 2;

    if (bitplanes > 1)
    {
      if ((bitplane_registers.bplcon0 & 0x8000) == 0x8000) // check if hires bit is set (bit 15 of register BPLCON0)
      {
        // high resolution
        dest_even = (ULO *)(line1 + graph_DDF_start + evenhiscroll);
      }
      else
      {
        // low resolution
        dest_even = (ULO *)(line1 + graph_DDF_start + evenscroll);
      }
      end_even = dest_even + bpl_length_in_bytes * 2;
    }

    switch (bitplanes)
    {
      case 6: pt6_tmp = memory_chip + bitplane_registers.bplpt[5];
      case 5: pt5_tmp = memory_chip + bitplane_registers.bplpt[4];
      case 4: pt4_tmp = memory_chip + bitplane_registers.bplpt[3];
      case 3: pt3_tmp = memory_chip + bitplane_registers.bplpt[2];
      case 2: pt2_tmp = memory_chip + bitplane_registers.bplpt[1];
      case 1: pt1_tmp = memory_chip + bitplane_registers.bplpt[0];
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
      dest_tmp[0] = Planar2ChunkyDecoder::P2COdd1(dat1, dat3, dat5);
      dest_tmp[1] = Planar2ChunkyDecoder::P2COdd2(dat1, dat3, dat5);
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
        dest_tmp[0] |= Planar2ChunkyDecoder::P2CEven1(dat2, dat4, dat6);
        dest_tmp[1] |= Planar2ChunkyDecoder::P2CEven2(dat2, dat4, dat6);
      }
    }
  }
  bitplane_registers.LinearAddModulo(bitplanes, bpl_length_in_bytes);
}

static void graphDecodeDualGeneric(int bitplanes)
{
  ULO bpl_length_in_bytes = graph_DDF_word_count * 2;
  if (bitplanes == 0) return;
  if (bpl_length_in_bytes != 0)
  {
    ULO *dest_even;
    ULO *dest_tmp;
    ULO *end_even;
    UBY *pt1_tmp, *pt2_tmp, *pt3_tmp, *pt4_tmp, *pt5_tmp, *pt6_tmp;
    ULO dat1, dat2, dat3, dat4, dat5, dat6;

    UBY *line1;
    UBY *line2;

    dat1 = dat2 = dat3 = dat4 = dat5 = dat6 = 0;

    graphSetLinePointers(&line1, &line2);

    // clear edges
    int maxscroll = (evenscroll > oddscroll) ? evenscroll : oddscroll;

    ULO temp = 0;
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
    ULO *dest_odd = (ULO *)(line1 + graph_DDF_start + oddscroll);
    ULO *end_odd = dest_odd + bpl_length_in_bytes * 2;

    if (bitplanes > 1)
    {
      // low resolution
      dest_even = (ULO *)(line2 + graph_DDF_start + evenscroll);
      end_even = dest_even + bpl_length_in_bytes * 2;
    }

    switch (bitplanes)
    {
      case 6: pt6_tmp = memory_chip + bitplane_registers.bplpt[5];
      case 5: pt5_tmp = memory_chip + bitplane_registers.bplpt[4];
      case 4: pt4_tmp = memory_chip + bitplane_registers.bplpt[3];
      case 3: pt3_tmp = memory_chip + bitplane_registers.bplpt[2];
      case 2: pt2_tmp = memory_chip + bitplane_registers.bplpt[1];
      case 1: pt1_tmp = memory_chip + bitplane_registers.bplpt[0];
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
      dest_tmp[0] = Planar2ChunkyDecoder::P2CDual1(dat1, dat3, dat5);
      dest_tmp[1] = Planar2ChunkyDecoder::P2CDual2(dat1, dat3, dat5);
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
        dest_tmp[0] = Planar2ChunkyDecoder::P2CDual1(dat2, dat4, dat6);
        dest_tmp[1] = Planar2ChunkyDecoder::P2CDual2(dat2, dat4, dat6);
      }
    }
  }
  bitplane_registers.LinearAddModulo(bitplanes, bpl_length_in_bytes);
}

//=======================================================
// Planar to chunky conversion, 1 bitplane hires or lores
//=======================================================

void graphDecode0()
{
  graphDecodeGeneric(0);
}

//=======================================================
// Planar to chunky conversion, 1 bitplane hires or lores
//=======================================================

void graphDecode1()
{
  graphDecodeGeneric(1);
}

//========================================================
// Planar to chunky conversion, 2 bitplanes hires or lores
//========================================================

void graphDecode2()
{
  graphDecodeGeneric(2);
}

//========================================================
// Planar to chunky conversion, 3 bitplanes hires or lores
//========================================================

void graphDecode3()
{
  graphDecodeGeneric(3);
}

//========================================================
// Planar to chunky conversion, 4 bitplanes hires or lores
//========================================================

void graphDecode4()
{
  graphDecodeGeneric(4);
}

//========================================================
// Planar to chunky conversion, 5 bitplanes hires or lores
//========================================================

void graphDecode5()
{
  graphDecodeGeneric(5);
}

//========================================================
// Planar to chunky conversion, 6 bitplanes hires or lores
//========================================================

void graphDecode6()
{
  graphDecodeGeneric(6);
}

//===============================================================
// Planar to chunky conversion, 2 bitplanes lores, dual playfield
//===============================================================
void graphDecode2Dual()
{
  graphDecodeDualGeneric(2);
}

//===============================================================
// Planar to chunky conversion, 3 bitplanes lores, dual playfield
//===============================================================
void graphDecode3Dual()
{
  graphDecodeDualGeneric(3);
}

//===============================================================
// Planar to chunky conversion, 4 bitplanes lores, dual playfield
//===============================================================
void graphDecode4Dual()
{
  graphDecodeDualGeneric(4);
}

//===============================================================
// Planar to chunky conversion, 5 bitplanes lores, dual playfield
//===============================================================
void graphDecode5Dual()
{
  graphDecodeDualGeneric(5);
}

//===============================================================
// Planar to chunky conversion, 6 bitplanes lores, dual playfield
//===============================================================
void graphDecode6Dual()
{
  graphDecodeDualGeneric(6);
}

//===========================================================================
// Calculate all variables connected to the screen emulation
// First calculate the ddf variables graph_DDF_start and graph_DDF_word_count
//   Check if stop is bigger than start
//   Check if strt and stop is mod 8 aligned
//   If NOK then set numberofwords and startpos to 0 and
//   DIWvisible vars to 128
//===========================================================================

void graphCalculateWindow()
{
  if ((bitplane_registers.bplcon0 & 0x8000) == 0x8000) // check if Hires bit is set (bit 15 of BPLCON0)
  {
    graphCalculateWindowHires();
  }
  else
  {
    ULO ddfstrt_limit = ((bitplane_registers.ddfstrt & 0xfc) < 0x18) ? 0x18 : (bitplane_registers.ddfstrt & 0xfc);
    ULO ddfstop_limit = ((bitplane_registers.ddfstop & 0xfc) > 0xd8) ? 0xd8 : (bitplane_registers.ddfstop & 0xfc);

    // set DDF (Display Data Fetch Start) variables
    if (ddfstop_limit < ddfstrt_limit)
    {
      graph_DDF_start = graph_DDF_word_count = 0;
      graph_DIW_first_visible = graph_DIW_last_visible = 256;
      return;
    }

    ULO ddfstop_aligned = ddfstop_limit & 0x07;
    ULO ddfstrt_aligned = ddfstrt_limit & 0x07;

    if (ddfstop_limit >= ddfstrt_limit)
    {
      graph_DDF_word_count = ((ddfstop_limit - ddfstrt_limit) >> 3) + 1;
    }
    else
    {
      graph_DDF_word_count = ((0xd8 - ddfstrt_limit) >> 3) + 1;
      ddfstop_aligned = 0;
    }

    if (ddfstop_aligned != ddfstrt_aligned)
    {
      graph_DDF_word_count++;
    }

    graph_DDF_start = ((ddfstrt_limit << 1) + 0x11); // 0x11 = 17 pixels = 8.5 bus cycles

    // set DIW (Display Window Start) variables (only used with drawing routines)

    if (diwxleft >= diwxright)
    {
      graph_DDF_start = graph_DDF_word_count = 0;
      graph_DIW_first_visible = graph_DIW_last_visible = 256;
    }

    const auto &chipsetMaxClip = Draw.GetChipsetBufferMaxClip();
    graph_DIW_first_visible = chipsetMaxClip.Left / 4;
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

    ULO last_position_in_line = (graph_DDF_word_count << 4) + graph_DDF_start;
    if (oddscroll > evenscroll)
    {
      last_position_in_line += oddscroll;
    }
    else
    {
      last_position_in_line += evenscroll;
    }

    graph_DIW_last_visible = chipsetMaxClip.Right;
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
  ULO ddfstrt_limit = ((bitplane_registers.ddfstrt & 0xfc) < 0x18) ? 0x18 : (bitplane_registers.ddfstrt & 0xfc);
  ULO ddfstop_limit = ((bitplane_registers.ddfstop & 0xfc) > 0xd8) ? 0xd8 : (bitplane_registers.ddfstop & 0xfc);

  if (ddfstrt_limit > ddfstop_limit)
  {
    graph_DDF_start = graph_DDF_word_count = 0;
    graph_DIW_first_visible = graph_DIW_last_visible = 256;
    return;
  }

  if (ddfstop_limit >= ddfstrt_limit)
  {
    graph_DDF_word_count = (((ddfstop_limit - ddfstrt_limit) + 15) >> 2) & 0x0FFFFFFFE;
  }
  else
  {
    graph_DDF_word_count = (((0xd8 - ddfstrt_limit) + 15) >> 2) & 0x0FFFFFFFE;
  }

  graph_DDF_start = (ddfstrt_limit << 2) + 18;

  // DDF variables done, now check DIW variables

  if ((diwxleft << 1) >= (diwxright << 1))
  {
    graph_DDF_start = graph_DDF_word_count = 0;
    graph_DIW_first_visible = graph_DIW_last_visible = 256;
  }

  const auto &chipsetMaxClip = Draw.GetChipsetBufferMaxClip();
  const ULO clip_left = chipsetMaxClip.Left / 2;
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

  ULO last_position_in_line = graph_DDF_start + (graph_DDF_word_count << 4);
  if (oddhiscroll > evenhiscroll)
  {
    last_position_in_line += oddhiscroll;
  }
  else
  {
    last_position_in_line += evenhiscroll;
  }

  const ULO clip_right = chipsetMaxClip.Right / 2;
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
  ULO currentY = scheduler.GetRasterY();
  if (graph_playfield_on)
  {
    // Playfield on, check if top has moved below graph_raster_y
    // Playfield on, check if playfield is turned off at this line
    if (!(currentY != diwybottom))
    {
      graph_playfield_on = false;
    }
  }
  else
  {
    // Playfield off, Check if playfield is turned on at this line
    if (!(currentY != diwytop))
    {
      // Don't turn on when top > bottom
      if (diwytop < diwybottom)
      {
        graph_playfield_on = true;
      }
    }
  }
  // OK, here the state of the playfield is taken care of
}

void graphDecodeNOP()
{
  bitplane_registers.LinearAddModulo((bitplane_registers.bplcon0 >> 12) & 0x07, graph_DDF_word_count * 2);
}

//-------------------------------------------------------------------------------
// Copy color block to line description
//-------------------------------------------------------------------------------

void graphLinedescColors(graph_line *current_graph_line)
{
  const ULL *hostColors = Draw.GetHostColors();
  for (auto color = 0; color < 64; color++)
  {
    current_graph_line->colors[color] = hostColors[color];
  }
}

//---------------------------------
// Sets line routines for this line
//---------------------------------

void graphLinedescRoutines(graph_line *current_graph_line)
{
  //=====================
  // Set drawing routines
  //=====================
  current_graph_line->draw_line_routine = draw_line_routine;
  current_graph_line->draw_line_BPL_res_routine = draw_line_BPL_res_routine;
}

//--------------------------------------------
// Sets line geometry data in line description
//--------------------------------------------

void graphLinedescGeometry(graph_line *current_graph_line)
{
  const auto &chipsetMaxClip = Draw.GetChipsetBufferMaxClip();
  ULO local_graph_DIW_first_visible = graph_DIW_first_visible;
  LON local_graph_DIW_last_visible = (LON)graph_DIW_last_visible;
  ULO local_graph_DDF_start = graph_DDF_start;
  ULO local_draw_left = chipsetMaxClip.Left / 4;
  ULO local_draw_right = chipsetMaxClip.Right / 4;
  ULO shift = 0;

  //=========================================================
  // Calculate first and last visible DIW and DIW pixel count
  //=========================================================

  if ((bitplane_registers.bplcon0 & 0x8000) != 0)
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
  if (local_graph_DIW_last_visible > (LON)local_draw_right)
  {
    local_graph_DIW_last_visible = (LON)local_draw_right;
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
  current_graph_line->MaxClipWidth = chipsetMaxClip.GetWidth();
  local_graph_DIW_first_visible >>= shift;
  local_graph_DIW_last_visible >>= shift;

  //======================================
  // Calculate BG front and back pad count
  //======================================

  local_draw_right -= local_graph_DIW_first_visible;
  local_graph_DIW_first_visible -= local_draw_left;
  local_draw_right -= local_graph_DIW_last_visible;
  current_graph_line->BG_pad_front = local_graph_DIW_first_visible;
  current_graph_line->BG_pad_back = local_draw_right;

  //======================================================
  // Need to remember playfield priorities to sort dual pf
  //======================================================

  current_graph_line->bplcon2 = bitplane_registers.bplcon2;
}

//---------------------------------------
// Smart sets line routines for this line
// Return TRUE if routines have changed
//---------------------------------------
BOOLE graphLinedescRoutinesSmart(graph_line *current_graph_line)
{
  //=====================
  // Set drawing routines
  //=====================

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

//-------------------------------------------------------------------------------
// Sets line geometry data in line description
// Return TRUE if geometry has changed
//-------------------------------------------------------------------------------
BOOLE graphLinedescGeometrySmart(graph_line *current_graph_line)
{
  const auto &chipsetMaxClip = Draw.GetChipsetBufferMaxClip();
  ULO local_graph_DIW_first_visible = graph_DIW_first_visible;
  LON local_graph_DIW_last_visible = (LON)graph_DIW_last_visible;
  ULO local_graph_DDF_start = graph_DDF_start;
  ULO local_draw_left = chipsetMaxClip.Left / 4;
  ULO local_draw_right = chipsetMaxClip.Right / 4;
  ULO shift = 0;
  BOOLE line_desc_changed = FALSE;

  //=========================================================
  // Calculate first and last visible DIW and DIW pixel count
  //=========================================================

  if ((bitplane_registers.bplcon0 & 0x8000) != 0)
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
  if (local_graph_DIW_last_visible > (LON)local_draw_right)
  {
    local_graph_DIW_last_visible = (LON)local_draw_right;
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
  current_graph_line->MaxClipWidth = chipsetMaxClip.GetWidth();
  local_graph_DIW_first_visible >>= shift;
  local_graph_DIW_last_visible >>= shift;

  //======================================
  // Calculate BG front and back pad count
  //======================================

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

  //======================================================
  // Need to remember playfield priorities to sort dual pf
  //======================================================

  if (current_graph_line->bplcon2 != bitplane_registers.bplcon2)
  {
    line_desc_changed = TRUE;
  }
  current_graph_line->bplcon2 = bitplane_registers.bplcon2;
  return line_desc_changed;
}

//-------------------------------------------
// Smart copy color block to line description
// Return TRUE if colors have changed
//-------------------------------------------

BOOLE graphLinedescColorsSmart(graph_line *current_graph_line)
{
  BOOLE result = FALSE;
  const ULL *hostColors = Draw.GetHostColors();

  // check full brightness colors
  for (ULO i = 0; i < 32; i++)
  {
    if (hostColors[i] != current_graph_line->colors[i])
    {
      current_graph_line->colors[i] = hostColors[i];
      current_graph_line->colors[i + 32] = hostColors[i + 32];
      result = TRUE;
    }
  }
  return result;
}

//---------------------------------------------------------------------
// Copy remainder of the data, data changed, no need to compare anymore
// Return TRUE = not equal
//---------------------------------------------------------------------

static BOOLE graphCompareCopyRest(ULO first_pixel, LON pixel_count, UBY *dest_line, const UBY *source_line)
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
    *((ULO *)(dest_line + first_pixel)) = *((ULO *)(source_line + first_pixel));
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

//--------------------------------------
// Copy data and compare
// Return TRUE = not equal FALSE = equal
//--------------------------------------

static BOOLE graphCompareCopy(ULO first_pixel, LON pixel_count, UBY *dest_line, UBY *source_line)
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
      if (*((ULO *)(source_line + first_pixel)) == *((ULO *)(dest_line + first_pixel)))
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
  const ULL hostColor0 = Draw.GetHostColors()[0];

  if (hostColor0 == current_graph_line->colors[0])
  {
    if (current_graph_line->linetype == graph_linetypes::GRAPH_LINE_SKIP)
    {
      // Already skipping this line and no changes
      return FALSE;
    }
    if (current_graph_line->linetype == graph_linetypes::GRAPH_LINE_BG)
    {
      //=============================================================
      // This line was a "background" line during the last drawing of
      // this frame buffer,
      // and it has the same color as last time.
      // We might be able to skip drawing it, we need to check the
      // flag that says it has been drawn in all of our buffers.
      //========================================================
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

  //=============================================================
  // This background line is different from the one in the buffer
  //=============================================================
  current_graph_line->linetype = graph_linetypes::GRAPH_LINE_BG;
  current_graph_line->colors[0] = hostColor0;
  current_graph_line->frames_left_until_BG_skip = Draw.GetBufferCount() - 1;
  graphLinedescRoutines(current_graph_line);
  return TRUE;
}

BOOLE graphLinedescSetBitplaneLine(graph_line *current_graph_line)
{
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

//---------------------------------------
// Smart makes a description of this line
// Return TRUE if linedesc has changed
//---------------------------------------

BOOLE graphLinedescMakeSmart(graph_line *current_graph_line)
{
  //=======================================
  // Is this a bitplane or background line?
  //=======================================
  if (draw_line_BG_routine != draw_line_routine)
  {
    return graphLinedescSetBitplaneLine(current_graph_line);
  }

  return graphLinedescSetBackgroundLine(current_graph_line);
}

//=============================================
// Smart compose the visible layout of the line
//=============================================

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
    line_desc_changed |= graphCompareCopy(current_graph_line->DIW_first_draw, (LON)(current_graph_line->DIW_pixel_count), current_graph_line->line1, graph_line1_tmp);

    // if the line is dual playfield, compare second playfield too
    if ((bitplane_registers.bplcon0 & 0x0400) != 0x0)
    {
      line_desc_changed |= graphCompareCopy(current_graph_line->DIW_first_draw, (LON)(current_graph_line->DIW_pixel_count), current_graph_line->line2, graph_line2_tmp);
    }

    if (current_graph_line->has_ham_sprites_online)
    {
      // Compare will not detect old HAM sprites by looking at pixel data etc. Always mark line as dirty.
      line_desc_changed = TRUE;
      current_graph_line->has_ham_sprites_online = false;
    }

    // add sprites to the line image
    if (line_exact_sprites->HasSpritesOnLine())
    {
      line_desc_changed = TRUE;
      line_exact_sprites->Merge(current_graph_line);
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

//============================
// Decode tables
// Called from the draw module
//============================

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

//=================================
// End of line handler for graphics
//=================================

void graphEndOfLine()
{
  if (chipsetIsCycleExact())
  {
    return;
  }

  // skip this frame?
  if (!Draw.ShouldSkipFrame())
  {
    unsigned int currentY = scheduler.GetRasterY();
    // update diw state
    graphPlayfieldOnOff();

    // skip lines before line $12
    if (currentY >= 0x12)
    {
      // make pointer to linedesc for this line
      graph_line *current_graph_line = graphGetLineDesc(Draw.GetDrawBufferIndex(), currentY);

      // decode sprites if DMA is enabled and raster is after line $18
      if ((dmacon & 0x20) == 0x20)
      {
        if (currentY >= 0x18)
        {
          line_exact_sprites->DMASpriteHandler();
          line_exact_sprites->ProcessActionList();
        }
      }

      // sprites decoded, sprites_onlineflag is set if there are any
      // update pointer to drawing routine
      Draw.UpdateDrawFunctions();

      // check if we are clipped
      // TODO: Might need odd line handling
      const auto &chipsetMaxClip = Draw.GetChipsetBufferMaxClip();
      unsigned int bottomLine = chipsetMaxClip.Bottom / 2;
      if ((currentY >= (chipsetMaxClip.Top / 2)) || (currentY < bottomLine))
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

      if (currentY == (scheduler.GetLinesInFrame() - 1))
      {
        // In the case when the display has more lines than the frame (AF or short frames)
        // this routine pads the remaining lines with background color
        for (unsigned int y = currentY + 1; y < bottomLine; ++y)
        {
          graph_line *graph_line_y = graphGetLineDesc(Draw.GetDrawBufferIndex(), y);
          graphLinedescSetBackgroundLine(graph_line_y);
        }
      }
    }
  }
}

void graphEndOfFrame()
{
  graph_playfield_on = false;
  if (graph_buffer_lost == true)
  {
    graphLineDescClear();
    graph_buffer_lost = false;
  }
}

void graphHardReset()
{
  graphIORegistersClear();
  graphLineDescClear();
}

void graphEmulationStart()
{
  graph_buffer_lost = false;
  graphLineDescClear();
  graphIOHandlersInstall();
}

void graphEmulationStop()
{
}

void graphStartup()
{
  graphP2CFunctionsInit();
  graphLineDescClear();
  graphIORegistersClear();
}

void graphShutdown()
{
}

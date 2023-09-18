/*=========================================================================*/
/* Fellow                                                                  */
/* Clock functions                                                         */
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

#include "RtcOkiMsm6242rs.h"
#include "FMEM.H"
#include "FELLOW.H"
#include "rtc.h"

bool rtc_enabled = false;
RtcOkiMsm6242rs rtc;

uint8_t rtcReadByte(uint32_t address)
{
  uint16_t result = rtc.read(address);
  uint8_t byte_result = (uint8_t) ((address & 1) ? result : (result>>8));

#ifdef RTC_LOG
  fellowAddLog("RTC Byte Read: %.8X, returned %.2X\n", address, byte_result);
#endif

  return byte_result;
}

uint16_t rtcReadWord(uint32_t address)
{
  uint16_t result = rtc.read(address);

#ifdef RTC_LOG
  fellowAddLog("RTC Word Read: %.8X, returned %.4X\n", address, result);
#endif

  return result;
}

uint32_t rtcReadLong(uint32_t address)
{
  uint32_t w1 = (uint32_t) rtc.read(address);
  uint32_t w2 = (uint32_t) rtc.read(address+2);
  uint32_t result = (w1 << 16) | w2;

#ifdef RTC_LOG
  fellowAddLog("RTC Long Read: %.8X, returned %.8X\n", address, result);
#endif

  return result;
}

void rtcWriteByte(uint8_t data, uint32_t address)
{
  rtc.write(data, address);

#ifdef RTC_LOG
  fellowAddLog("RTC Byte Write: %.8X %.2X\n", address, data);
#endif
}

void rtcWriteWord(uint16_t data, uint32_t address)
{
  rtc.write(data, address);

#ifdef RTC_LOG
  fellowAddLog("RTC Word Write: %.8X %.4X\n", address, data);
#endif
}

void rtcWriteLong(uint32_t data, uint32_t address)
{
  rtc.write(data, address);
  rtc.write(data, address + 2);

#ifdef RTC_LOG
  fellowAddLog("RTC Long Write: %.8X %.8X\n", address, data);
#endif
}

bool rtcSetEnabled(bool enabled)
{
  bool needreset = (rtc_enabled != enabled);
  rtc_enabled = enabled;
  return needreset;
}

bool rtcGetEnabled(void)
{
  return rtc_enabled;
}

void rtcMap(void)
{
  if (rtcGetEnabled())
  {
    memoryBankSet(rtcReadByte,
      rtcReadWord,
      rtcReadLong,
      rtcWriteByte, 
      rtcWriteWord, 
      rtcWriteLong,
      nullptr, 
      0xdc, 
      0,
      FALSE);

#ifdef RTC_LOG
  fellowAddLog("Mapped RTC at $DC0000\n");
#endif
  }
}


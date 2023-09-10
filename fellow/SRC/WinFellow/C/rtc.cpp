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

UBY rtcReadByte(ULO address)
{
  UWO result = rtc.read(address);
  UBY byte_result = (UBY) ((address & 1) ? result : (result>>8));

#ifdef RTC_LOG
  fellowAddLog("RTC Byte Read: %.8X, returned %.2X\n", address, byte_result);
#endif

  return byte_result;
}

UWO rtcReadWord(ULO address)
{
  UWO result = rtc.read(address);

#ifdef RTC_LOG
  fellowAddLog("RTC Word Read: %.8X, returned %.4X\n", address, result);
#endif

  return result;
}

ULO rtcReadLong(ULO address)
{
  ULO w1 = (ULO) rtc.read(address);
  ULO w2 = (ULO) rtc.read(address+2);
  ULO result = (w1 << 16) | w2;

#ifdef RTC_LOG
  fellowAddLog("RTC Long Read: %.8X, returned %.8X\n", address, result);
#endif

  return result;
}

void rtcWriteByte(UBY data, ULO address)
{
  rtc.write(data, address);

#ifdef RTC_LOG
  fellowAddLog("RTC Byte Write: %.8X %.2X\n", address, data);
#endif
}

void rtcWriteWord(UWO data, ULO address)
{
  rtc.write(data, address);

#ifdef RTC_LOG
  fellowAddLog("RTC Word Write: %.8X %.4X\n", address, data);
#endif
}

void rtcWriteLong(ULO data, ULO address)
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
      NULL, 
      0xdc, 
      0,
      FALSE);

#ifdef RTC_LOG
  fellowAddLog("Mapped RTC at $DC0000\n");
#endif
  }
}


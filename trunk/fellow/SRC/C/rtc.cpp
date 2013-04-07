#include "RtcOkiMsm6242rs.h"
#include "FMEM.H"
#include "FELLOW.H"

RtcOkiMsm6242rs rtc;

UBY rtcReadByte(ULO address)
{
  UWO result = rtc.read(address);
  UBY byte_result = (UBY) ((address & 1) ? result : (result>>8));

  fellowAddLog("RTC Byte Read: %.8X, returned %.2X\n", address, byte_result);

  return byte_result;
}

UWO rtcReadWord(ULO address)
{
  UWO result = rtc.read(address);

  fellowAddLog("RTC Word Read: %.8X, returned %.4X\n", address, result);

  return result;
}

ULO rtcReadLong(ULO address)
{
  ULO w1 = (ULO) rtc.read(address);
  ULO w2 = (ULO) rtc.read(address+2);
  ULO result = (w1 << 16) | w2;

  fellowAddLog("RTC Long Read: %.8X, returned %.8X\n", address, result);

  return result;
}

void rtcWriteByte(UBY data, ULO address)
{
  rtc.write(data, address);
  fellowAddLog("RTC Byte Write: %.8X %.2X\n", address, data);
}

void rtcWriteWord(UWO data, ULO address)
{
  rtc.write(data, address);
  fellowAddLog("RTC Word Write: %.8X %.4X\n", address, data);
}

void rtcWriteLong(ULO data, ULO address)
{
  rtc.write(data, address);
  rtc.write(data, address + 2);
  fellowAddLog("RTC Long Write: %.8X %.8X\n", address, data);
}

void rtcMap(void)
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
}

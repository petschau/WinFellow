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

#include <ctime>
#include "rtc.h"
#include "RtcOkiMsm6242rs.h"
#include "FELLOW.H"

int RtcOkiMsm6242rs::GetRegisterNumberFromAddress(uint32_t address)
{
  return (address >> 2) & 0xf;
}

struct tm *RtcOkiMsm6242rs::GetCurrentOrHeldTime(void)
{
  // Add the time passed since the clock was last set
  time_t actual_now = time(nullptr);
  double time_passed = difftime(actual_now, _rtcLastActualTime);
  time_t rtc_now = _rtcTime + (time_t) time_passed;
  return localtime(&rtc_now);
}

void RtcOkiMsm6242rs::SetCurrentTime(struct tm *datetime)
{
  _rtcTime = mktime(datetime);
  _rtcLastActualTime = time(nullptr);
}

void RtcOkiMsm6242rs::ReplaceFirstDigit(int& value, int new_digit)
{
  value = value - (value % 10) + new_digit;
}

void RtcOkiMsm6242rs::ReplaceSecondDigit(int& value, int new_digit)
{
  value = (value % 10) + (new_digit % 10)*10;
}

void RtcOkiMsm6242rs::ReplaceSecondDigitAllowBCDOverflow(int& value, int new_digit)
{
  value = (value % 10) + (new_digit & 0xf)*10;
}

uint16_t RtcOkiMsm6242rs::GetFirstDigit(int value)
{
  return (uint16_t) (value % 10);
}

void RtcOkiMsm6242rs::SetFirstDigit(struct tm& datetime, int& value, uint16_t data)
{
  ReplaceFirstDigit(value, data);
  SetCurrentTime(&datetime);
}

uint16_t RtcOkiMsm6242rs::GetSecondDigit(int value)
{
  return (uint16_t) ((value / 10) % 10);
}

void RtcOkiMsm6242rs::SetSecondDigit(struct tm& datetime, int& value, uint16_t data)
{
  ReplaceSecondDigit(value, data);
  SetCurrentTime(&datetime);
}

uint16_t RtcOkiMsm6242rs::GetSecondRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetFirstDigit(datetime->tm_sec);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetSecondRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetFirstDigit(*datetime, datetime->tm_sec, data);
}

uint16_t RtcOkiMsm6242rs::GetTenSecondRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetSecondDigit(datetime->tm_sec);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetTenSecondRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetSecondDigit(*datetime, datetime->tm_sec, data);
}

uint16_t RtcOkiMsm6242rs::GetMinuteRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetFirstDigit(datetime->tm_min);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetMinuteRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetFirstDigit(*datetime, datetime->tm_min, data);
}

uint16_t RtcOkiMsm6242rs::GetTenMinuteRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetSecondDigit(datetime->tm_min);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetTenMinuteRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetSecondDigit(*datetime, datetime->tm_min, data);
}

uint16_t RtcOkiMsm6242rs::GetHourRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetFirstDigit(datetime->tm_hour);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetHourRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetFirstDigit(*datetime, datetime->tm_hour, data);
}

uint16_t RtcOkiMsm6242rs::GetTenHourRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetSecondDigit(datetime->tm_hour);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetTenHourRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetSecondDigit(*datetime, datetime->tm_hour, data);
}

uint16_t RtcOkiMsm6242rs::GetDayRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetFirstDigit(datetime->tm_mday);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetDayRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetFirstDigit(*datetime, datetime->tm_mday, data);
}

uint16_t RtcOkiMsm6242rs::GetTenDayRegister(void)
{
  tm *datetime = GetCurrentOrHeldTime();
  
  if(datetime)
    return GetSecondDigit(datetime->tm_mday);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetTenDayRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetSecondDigit(*datetime, datetime->tm_mday, data);
}

uint16_t RtcOkiMsm6242rs::GetMonthRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetFirstDigit(datetime->tm_mon + 1);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetMonthRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime) {
    int month = datetime->tm_mon + 1;
    ReplaceFirstDigit(month, data);
    datetime->tm_mon = month - 1;
    SetCurrentTime(datetime);
  }
}

uint16_t RtcOkiMsm6242rs::GetTenMonthRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetSecondDigit(datetime->tm_mon + 1);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetTenMonthRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime) {
    int month = datetime->tm_mon + 1;
    ReplaceSecondDigit(month, data);
    datetime->tm_mon = month - 1;
    SetCurrentTime(datetime);
  }
}

uint16_t RtcOkiMsm6242rs::GetYearRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return GetFirstDigit(datetime->tm_year);
  else
    return 0;
}

void RtcOkiMsm6242rs::SetYearRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    SetFirstDigit(*datetime, datetime->tm_year, data);
}

uint16_t RtcOkiMsm6242rs::GetTenYearRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime)
    return datetime->tm_year / 10;
  else
    return 0;
}

void RtcOkiMsm6242rs::SetTenYearRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime) {
    ReplaceSecondDigitAllowBCDOverflow(datetime->tm_year, data);

    // mktime's lower limit is 1970, if the year is less than 1970, assume they mean 21st century
    if (datetime->tm_year < 70)
    {
      datetime->tm_year += 100;
    }
    SetCurrentTime(datetime);
  }
}

uint16_t RtcOkiMsm6242rs::GetWeekRegister(void)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime) {
    int weekday = (datetime->tm_wday + _rtcWeekdayModifier) % 7;
    return GetFirstDigit(weekday);
  }
  else
    return 0;
}

void RtcOkiMsm6242rs::SetWeekRegister(uint16_t data)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime) {
    _rtcWeekdayModifier = (data % 10) - datetime->tm_wday;
    if (_rtcWeekdayModifier < 0)
    {
      _rtcWeekdayModifier += 7;
    }
  }
}

uint16_t RtcOkiMsm6242rs::GetControlRegisterD(void)
{
  return (_thirtySecAdjFlag << 3) | (_irqFlag << 2) | (_busyFlag << 1) | _holdFlag;
}

void RtcOkiMsm6242rs::SetControlRegisterD(uint16_t data)
{
  _thirtySecAdjFlag = (data & 0x8) >> 3;
  _busyFlag = (data & 0x2) >> 1;
  _holdFlag = data & 1;
}

uint16_t RtcOkiMsm6242rs::GetControlRegisterE(void)
{
  return (_t1Flag << 3) | (_t0Flag << 2) | (_itrptStdFlag << 1) | _maskFlag;
}

void RtcOkiMsm6242rs::SetControlRegisterE(uint16_t data)
{
  // We never act on these, keep them to be able to read or write them
  _t1Flag = (data & 0x8) >> 3;
  _t0Flag = (data & 0x4) >> 2;
  _itrptStdFlag = (data & 0x2) >> 1;
  _maskFlag = data & 1;
}

uint16_t RtcOkiMsm6242rs::GetControlRegisterF(void)
{
  return (_testFlag << 3) | (_twentyFourTwelveFlag << 2) | (_stopFlag << 1) | _restFlag;
}

void RtcOkiMsm6242rs::SetControlRegisterF(uint16_t data)
{
  _testFlag = (data & 0x8) >> 3;
  _twentyFourTwelveFlag = (data & 0x4) >> 2;
  _stopFlag = (data & 0x2) >> 1;
  _restFlag = data & 1;
}

uint16_t RtcOkiMsm6242rs::read(uint32_t address)
{
#ifdef RTC_LOG
  logRtcTime("read");
#endif

  int register_number = GetRegisterNumberFromAddress(address);
  return (this->*_registerGetters[register_number])();
}

void RtcOkiMsm6242rs::write(uint16_t data, uint32_t address)
{
#ifdef RTC_LOG
  logRtcTime("Before write");
#endif

  int register_number = GetRegisterNumberFromAddress(address);
  (this->*_registerSetters[register_number])(data);

#ifdef RTC_LOG
  logRtcTime("After write");
#endif
}

#ifdef RTC_LOG

void RtcOkiMsm6242rs::logRtcTime(char *msg)
{
  struct tm *datetime = GetCurrentOrHeldTime();

  if(datetime) {
    fellowAddLog("RTC %s: Year %d Month %d Monthday %d Actual-weekday %d RTC-weekday %d %.2d:%.2d.%.2d\n",
      msg,
      datetime->tm_year + 1900,
      datetime->tm_mon + 1,
      datetime->tm_mday,
      datetime->tm_wday,
      datetime->tm_wday + _rtcWeekdayModifier,
      datetime->tm_hour,
      datetime->tm_min,
      datetime->tm_sec);
  }
}

#endif

void RtcOkiMsm6242rs::InitializeRegisterGetters(void)
{
  _registerGetters[0] = &RtcOkiMsm6242rs::GetSecondRegister;
  _registerGetters[1] = &RtcOkiMsm6242rs::GetTenSecondRegister;
  _registerGetters[2] = &RtcOkiMsm6242rs::GetMinuteRegister;
  _registerGetters[3] = &RtcOkiMsm6242rs::GetTenMinuteRegister;
  _registerGetters[4] = &RtcOkiMsm6242rs::GetHourRegister;
  _registerGetters[5] = &RtcOkiMsm6242rs::GetTenHourRegister;
  _registerGetters[6] = &RtcOkiMsm6242rs::GetDayRegister;
  _registerGetters[7] = &RtcOkiMsm6242rs::GetTenDayRegister;
  _registerGetters[8] = &RtcOkiMsm6242rs::GetMonthRegister;
  _registerGetters[9] = &RtcOkiMsm6242rs::GetTenMonthRegister;
  _registerGetters[10] = &RtcOkiMsm6242rs::GetYearRegister;
  _registerGetters[11] = &RtcOkiMsm6242rs::GetTenYearRegister;
  _registerGetters[12] = &RtcOkiMsm6242rs::GetWeekRegister;
  _registerGetters[13] = &RtcOkiMsm6242rs::GetControlRegisterD;
  _registerGetters[14] = &RtcOkiMsm6242rs::GetControlRegisterE;
  _registerGetters[15] = &RtcOkiMsm6242rs::GetControlRegisterF;
}

void RtcOkiMsm6242rs::InitializeRegisterSetters(void)
{
  _registerSetters[0] = &RtcOkiMsm6242rs::SetSecondRegister;
  _registerSetters[1] = &RtcOkiMsm6242rs::SetTenSecondRegister;
  _registerSetters[2] = &RtcOkiMsm6242rs::SetMinuteRegister;
  _registerSetters[3] = &RtcOkiMsm6242rs::SetTenMinuteRegister;
  _registerSetters[4] = &RtcOkiMsm6242rs::SetHourRegister;
  _registerSetters[5] = &RtcOkiMsm6242rs::SetTenHourRegister;
  _registerSetters[6] = &RtcOkiMsm6242rs::SetDayRegister;
  _registerSetters[7] = &RtcOkiMsm6242rs::SetTenDayRegister;
  _registerSetters[8] = &RtcOkiMsm6242rs::SetMonthRegister;
  _registerSetters[9] = &RtcOkiMsm6242rs::SetTenMonthRegister;
  _registerSetters[10] = &RtcOkiMsm6242rs::SetYearRegister;
  _registerSetters[11] = &RtcOkiMsm6242rs::SetTenYearRegister;
  _registerSetters[12] = &RtcOkiMsm6242rs::SetWeekRegister;
  _registerSetters[13] = &RtcOkiMsm6242rs::SetControlRegisterD;
  _registerSetters[14] = &RtcOkiMsm6242rs::SetControlRegisterE;
  _registerSetters[15] = &RtcOkiMsm6242rs::SetControlRegisterF;
}

RtcOkiMsm6242rs::RtcOkiMsm6242rs(void)
{
  _rtcLastActualTime = _rtcTime = time(nullptr);
  _rtcWeekdayModifier = 0;

  _irqFlag = 0;
  _holdFlag = 0;
  _thirtySecAdjFlag = 0;
  _busyFlag = 0;
  _maskFlag = 0;
  _itrptStdFlag = 0;
  _t0Flag = 0;
  _t1Flag = 0;
  _restFlag = 0;
  _stopFlag = 0;
  _twentyFourTwelveFlag = 1;
  _testFlag = 0;

  InitializeRegisterGetters();
  InitializeRegisterSetters();
}

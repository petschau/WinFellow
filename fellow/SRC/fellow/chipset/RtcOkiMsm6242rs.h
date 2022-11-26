#pragma once

#include <time.h>
#include "fellow/api/defs.h"

class RtcOkiMsm6242rs;
typedef UWO (RtcOkiMsm6242rs::*RtcOkiMsm6242rsRegisterGetter)();
typedef void (RtcOkiMsm6242rs::*RtcOkiMsm6242rsRegisterSetter)(UWO data);

class RtcOkiMsm6242rs
{
private:
  RtcOkiMsm6242rsRegisterGetter _registerGetters[16];
  RtcOkiMsm6242rsRegisterSetter _registerSetters[16];
  time_t _rtcLastActualTime; // Timestamp for when _rtcTime was set. Used to calculate time passed since then.
  time_t _rtcTime;           // The RTC value as set by programs.
  int _rtcWeekdayModifier;   // The weekday difference set by a program.

  UWO _irqFlag;
  UWO _holdFlag;
  UWO _thirtySecAdjFlag;
  UWO _busyFlag;
  UWO _maskFlag;
  UWO _itrptStdFlag;
  UWO _t0Flag;
  UWO _t1Flag;
  UWO _restFlag;
  UWO _stopFlag;
  UWO _twentyFourTwelveFlag;
  UWO _testFlag;

  struct tm *GetCurrentOrHeldTime();
  void SetCurrentTime(struct tm *datetime);

  int GetRegisterNumberFromAddress(ULO address);

  UWO GetFirstDigit(int value);
  void SetFirstDigit(struct tm &datetime, int &value, UWO data);
  void ReplaceFirstDigit(int &value, int new_digit);

  UWO GetSecondDigit(int value);
  void SetSecondDigit(struct tm &datetime, int &value, UWO data);
  void ReplaceSecondDigit(int &value, int new_digit);
  void ReplaceSecondDigitAllowBCDOverflow(int &value, int new_digit);

  UWO GetSecondRegister();
  void SetSecondRegister(UWO data);
  UWO GetTenSecondRegister();
  void SetTenSecondRegister(UWO data);
  UWO GetMinuteRegister();
  void SetMinuteRegister(UWO data);
  UWO GetTenMinuteRegister();
  void SetTenMinuteRegister(UWO data);
  UWO GetHourRegister();
  void SetHourRegister(UWO data);
  UWO GetTenHourRegister();
  void SetTenHourRegister(UWO data);
  UWO GetDayRegister();
  void SetDayRegister(UWO data);
  UWO GetTenDayRegister();
  void SetTenDayRegister(UWO data);
  UWO GetMonthRegister();
  void SetMonthRegister(UWO data);
  UWO GetTenMonthRegister();
  void SetTenMonthRegister(UWO data);
  UWO GetYearRegister();
  void SetYearRegister(UWO data);
  UWO GetTenYearRegister();
  void SetTenYearRegister(UWO data);
  UWO GetWeekRegister();
  void SetWeekRegister(UWO data);
  UWO GetControlRegisterD();
  void SetControlRegisterD(UWO data);
  UWO GetControlRegisterE();
  void SetControlRegisterE(UWO data);
  UWO GetControlRegisterF();
  void SetControlRegisterF(UWO data);

  void InitializeRegisterGetters();
  void InitializeRegisterSetters();

public:
  UWO read(ULO address);
  void write(UWO data, ULO address);
  void logRtcTime(STR *msg);
  RtcOkiMsm6242rs();
};

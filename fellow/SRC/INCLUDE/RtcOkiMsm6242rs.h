#ifndef RTCOKIMSM6242RS_H
#define RTCOKIMSM6242RS_H

#include "DEFS.H"

class RtcOkiMsm6242rs;
typedef UWO (RtcOkiMsm6242rs::*RtcOkiMsm6242rsRegisterGetter)(void);
typedef void (RtcOkiMsm6242rs::*RtcOkiMsm6242rsRegisterSetter)(UWO data);

class RtcOkiMsm6242rs
{
private:
  RtcOkiMsm6242rsRegisterGetter _registerGetters[16];
  RtcOkiMsm6242rsRegisterSetter _registerSetters[16];
  time_t _rtcLastActualTime;  // Timestamp for when _rtcTime was set. Used to calculate time passed since then.
  time_t _rtcTime;	      // The RTC value as set by programs.
  int _rtcWeekdayModifier;    // The weekday difference set by a program.

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

  struct tm* GetCurrentOrHeldTime(void);
  void SetCurrentTime(struct tm *datetime);

  int GetRegisterNumberFromAddress(ULO address);

  UWO GetFirstDigit(int value);
  void SetFirstDigit(struct tm& datetime, int& value, UWO data);
  void ReplaceFirstDigit(int& value, int new_digit);

  UWO GetSecondDigit(int value);
  void SetSecondDigit(struct tm& datetime, int& value, UWO data);
  void ReplaceSecondDigit(int& value, int new_digit);
  void ReplaceSecondDigitAllowBCDOverflow(int& value, int new_digit);

  UWO GetSecondRegister(void);
  void SetSecondRegister(UWO data);
  UWO GetTenSecondRegister(void);
  void SetTenSecondRegister(UWO data);
  UWO GetMinuteRegister(void);
  void SetMinuteRegister(UWO data);
  UWO GetTenMinuteRegister(void);
  void SetTenMinuteRegister(UWO data);
  UWO GetHourRegister(void);
  void SetHourRegister(UWO data);
  UWO GetTenHourRegister(void);
  void SetTenHourRegister(UWO data);
  UWO GetDayRegister(void);
  void SetDayRegister(UWO data);
  UWO GetTenDayRegister(void);
  void SetTenDayRegister(UWO data);
  UWO GetMonthRegister(void);
  void SetMonthRegister(UWO data);
  UWO GetTenMonthRegister(void);
  void SetTenMonthRegister(UWO data);
  UWO GetYearRegister(void);
  void SetYearRegister(UWO data);
  UWO GetTenYearRegister(void);
  void SetTenYearRegister(UWO data);
  UWO GetWeekRegister(void);
  void SetWeekRegister(UWO data);
  UWO GetControlRegisterD(void);
  void SetControlRegisterD(UWO data);
  UWO GetControlRegisterE(void);
  void SetControlRegisterE(UWO data);
  UWO GetControlRegisterF(void);
  void SetControlRegisterF(UWO data);

  void InitializeRegisterGetters(void);
  void InitializeRegisterSetters(void);

public:
  UWO read(ULO address);
  void write(UWO data, ULO address);
  void logRtcTime(STR *msg);
  RtcOkiMsm6242rs(void);
};

#endif

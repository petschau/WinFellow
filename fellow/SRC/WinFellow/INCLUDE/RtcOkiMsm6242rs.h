#pragma once

#include "DEFS.H"

class RtcOkiMsm6242rs;
typedef uint16_t (RtcOkiMsm6242rs::*RtcOkiMsm6242rsRegisterGetter)();
typedef void (RtcOkiMsm6242rs::*RtcOkiMsm6242rsRegisterSetter)(uint16_t data);

class RtcOkiMsm6242rs
{
private:
  RtcOkiMsm6242rsRegisterGetter _registerGetters[16];
  RtcOkiMsm6242rsRegisterSetter _registerSetters[16];
  time_t _rtcLastActualTime;  // Timestamp for when _rtcTime was set. Used to calculate time passed since then.
  time_t _rtcTime;	      // The RTC value as set by programs.
  int _rtcWeekdayModifier;    // The weekday difference set by a program.

  uint16_t _irqFlag;
  uint16_t _holdFlag;
  uint16_t _thirtySecAdjFlag;
  uint16_t _busyFlag;
  uint16_t _maskFlag;
  uint16_t _itrptStdFlag;
  uint16_t _t0Flag;
  uint16_t _t1Flag;
  uint16_t _restFlag;
  uint16_t _stopFlag;
  uint16_t _twentyFourTwelveFlag;
  uint16_t _testFlag;

  struct tm* GetCurrentOrHeldTime();
  void SetCurrentTime(struct tm *datetime);

  int GetRegisterNumberFromAddress(uint32_t address);

  uint16_t GetFirstDigit(int value);
  void SetFirstDigit(struct tm& datetime, int& value, uint16_t data);
  void ReplaceFirstDigit(int& value, int new_digit);

  uint16_t GetSecondDigit(int value);
  void SetSecondDigit(struct tm& datetime, int& value, uint16_t data);
  void ReplaceSecondDigit(int& value, int new_digit);
  void ReplaceSecondDigitAllowBCDOverflow(int& value, int new_digit);

  uint16_t GetSecondRegister();
  void SetSecondRegister(uint16_t data);
  uint16_t GetTenSecondRegister();
  void SetTenSecondRegister(uint16_t data);
  uint16_t GetMinuteRegister();
  void SetMinuteRegister(uint16_t data);
  uint16_t GetTenMinuteRegister();
  void SetTenMinuteRegister(uint16_t data);
  uint16_t GetHourRegister();
  void SetHourRegister(uint16_t data);
  uint16_t GetTenHourRegister();
  void SetTenHourRegister(uint16_t data);
  uint16_t GetDayRegister();
  void SetDayRegister(uint16_t data);
  uint16_t GetTenDayRegister();
  void SetTenDayRegister(uint16_t data);
  uint16_t GetMonthRegister();
  void SetMonthRegister(uint16_t data);
  uint16_t GetTenMonthRegister();
  void SetTenMonthRegister(uint16_t data);
  uint16_t GetYearRegister();
  void SetYearRegister(uint16_t data);
  uint16_t GetTenYearRegister();
  void SetTenYearRegister(uint16_t data);
  uint16_t GetWeekRegister();
  void SetWeekRegister(uint16_t data);
  uint16_t GetControlRegisterD();
  void SetControlRegisterD(uint16_t data);
  uint16_t GetControlRegisterE();
  void SetControlRegisterE(uint16_t data);
  uint16_t GetControlRegisterF();
  void SetControlRegisterF(uint16_t data);

  void InitializeRegisterGetters();
  void InitializeRegisterSetters();

public:
  uint16_t read(uint32_t address);
  void write(uint16_t data, uint32_t address);
  void logRtcTime(char *msg);
  RtcOkiMsm6242rs();
};

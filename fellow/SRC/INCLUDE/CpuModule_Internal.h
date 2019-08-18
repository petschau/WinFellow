#pragma once

#include "fellow/api/defs.h"

// This header file defines the internal interfaces of the CPU module.
extern void cpuMakeOpcodeTableForModel();
extern void cpuCreateMulTimeTables();

// StackFrameGen
extern void cpuStackFrameGenerate(UWO vector_no, ULO pc);
extern void cpuStackFrameInit();

// Registers
extern ULO cpu_sr; // Not static because the flags calculation uses it extensively
extern BOOLE cpuGetFlagSupervisor();
extern BOOLE cpuGetFlagMaster();
extern void cpuSetUspDirect(ULO usp);
extern ULO cpuGetUspDirect();
extern ULO cpuGetUspAutoMap();
extern void cpuSetSspDirect(ULO ssp);
extern ULO cpuGetSspDirect();
extern ULO cpuGetSspAutoMap();
extern void cpuSetMspDirect(ULO msp);
extern ULO cpuGetMspDirect();
extern ULO cpuGetMspAutoMap();
extern void cpuSetMspAutoMap(ULO new_msp);
extern ULO cpuGetIspAutoMap();
extern void cpuSetIspAutoMap(ULO new_isp);
extern void cpuSetDReg(ULO i, ULO value);
extern ULO cpuGetDReg(ULO i);
extern void cpuSetAReg(ULO i, ULO value);
extern ULO cpuGetAReg(ULO i);
extern void cpuSetReg(ULO da, ULO i, ULO value);
extern ULO cpuGetReg(ULO da, ULO i);
extern void cpuSetPC(ULO address);
extern ULO cpuGetPC();
extern void cpuSetStop(BOOLE stop);
extern BOOLE cpuGetStop();
extern void cpuSetVbr(ULO vbr);
extern ULO cpuGetVbr();
extern void cpuSetSfc(ULO sfc);
extern ULO cpuGetSfc();
extern void cpuSetDfc(ULO dfc);
extern ULO cpuGetDfc();
extern void cpuSetCacr(ULO cacr);
extern ULO cpuGetCacr();
extern void cpuSetCaar(ULO caar);
extern ULO cpuGetCaar();
extern void cpuSetSR(ULO sr);
extern ULO cpuGetSR();
extern void cpuSetInstructionTime(ULO cycles);
extern ULO cpuGetInstructionTime();
extern void cpuSetOriginalPC(ULO pc);
extern ULO cpuGetOriginalPC();

#ifdef CPU_INSTRUCTION_LOGGING

extern void cpuSetCurrentOpcode(UWO opcode);
extern UWO cpuGetCurrentOpcode();

#endif

extern void cpuProfileWrite();

extern void cpuSetModelMask(UBY model_mask);
extern UBY cpuGetModelMask();
extern void cpuSetDRegWord(ULO regno, UWO val);
extern void cpuSetDRegByte(ULO regno, UBY val);
extern UWO cpuGetRegWord(ULO i, ULO regno);
extern UWO cpuGetDRegWord(ULO regno);
extern UBY cpuGetDRegByte(ULO regno);
extern ULO cpuGetDRegWordSignExtLong(ULO regno);
extern UWO cpuGetDRegByteSignExtWord(ULO regno);
extern ULO cpuGetDRegByteSignExtLong(ULO regno);
extern UWO cpuGetARegWord(ULO regno);
extern UBY cpuGetARegByte(ULO regno);

extern UWO cpuGetNextWord();
extern ULO cpuGetNextWordSignExt();
extern ULO cpuGetNextLong();
extern void cpuSkipNextWord();
extern void cpuSkipNextLong();
extern void cpuClearPrefetch();
extern void cpuValidateReadPointer();

extern void cpuInitializeFromNewPC(ULO new_pc);

// Effective address
extern ULO cpuEA02(ULO regno);
extern ULO cpuEA03(ULO regno, ULO size);
extern ULO cpuEA04(ULO regno, ULO size);
extern ULO cpuEA05(ULO regno);
extern ULO cpuEA06(ULO regno);
extern ULO cpuEA70();
extern ULO cpuEA71();
extern ULO cpuEA72();
extern ULO cpuEA73();

// Flags
extern void cpuSetFlagsAdd(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsSub(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsCmp(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetZFlagBitOpsB(UBY res);
extern void cpuSetZFlagBitOpsL(ULO res);

extern void cpuSetFlagsNZ00NewB(UBY res);
extern void cpuSetFlagsNZ00NewW(UWO res);
extern void cpuSetFlagsNZ00NewL(ULO res);
extern void cpuSetFlagsNZ00New64(LLO res);

extern void cpuSetFlagZ(BOOLE f);
extern void cpuSetFlagN(BOOLE f);
extern void cpuSetFlagV(BOOLE f);
extern void cpuSetFlagC(BOOLE f);
extern void cpuSetFlagXC(BOOLE f);
extern void cpuSetFlags0100();
extern void cpuSetFlagsNeg(BOOLE z, BOOLE rm, BOOLE dm);
extern BOOLE cpuGetFlagX();
extern void cpuSetFlagsNegx(BOOLE z, BOOLE rm, BOOLE dm);
extern BOOLE cpuGetFlagV();
extern void cpuSetFlagsNZVC(BOOLE z, BOOLE n, BOOLE v, BOOLE c);
extern void cpuSetFlagsVC(BOOLE v, BOOLE c);
extern void cpuSetFlagsShiftZero(BOOLE z, BOOLE rm);
extern void cpuSetFlagsShift(BOOLE z, BOOLE rm, BOOLE c, BOOLE v);
extern void cpuSetFlagsRotate(BOOLE z, BOOLE rm, BOOLE c);
extern void cpuSetFlagsRotateX(UWO z, UWO rm, UWO x);
extern void cpuSetFlagsAddX(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsSubX(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsAbs(UWO f);
extern UWO cpuGetZFlagB(UBY res);
extern UWO cpuGetZFlagW(UWO res);
extern UWO cpuGetZFlagL(ULO res);
extern UWO cpuGetNFlagB(UBY res);
extern UWO cpuGetNFlagW(UWO res);
extern UWO cpuGetNFlagL(ULO res);
extern void cpuClearFlagsVC();

extern BOOLE cpuCalculateConditionCode0();
extern BOOLE cpuCalculateConditionCode1();
extern BOOLE cpuCalculateConditionCode2();
extern BOOLE cpuCalculateConditionCode3();
extern BOOLE cpuCalculateConditionCode4();
extern BOOLE cpuCalculateConditionCode5();
extern BOOLE cpuCalculateConditionCode6();
extern BOOLE cpuCalculateConditionCode7();
extern BOOLE cpuCalculateConditionCode8();
extern BOOLE cpuCalculateConditionCode9();
extern BOOLE cpuCalculateConditionCode10();
extern BOOLE cpuCalculateConditionCode11();
extern BOOLE cpuCalculateConditionCode12();
extern BOOLE cpuCalculateConditionCode13();
extern BOOLE cpuCalculateConditionCode14();
extern BOOLE cpuCalculateConditionCode15();
extern BOOLE cpuCalculateConditionCode(ULO cc);

// Logging
#ifdef CPU_INSTRUCTION_LOGGING
extern void cpuCallInstructionLoggingFunc();
extern void cpuCallExceptionLoggingFunc(const STR *description, ULO original_pc, UWO opcode);
extern void cpuCallInterruptLoggingFunc(ULO level, ULO vector_address);
#endif

// Interrupt
extern ULO cpuActivateSSP();
extern void cpuSetRaiseInterrupt(BOOLE raise_irq);
extern BOOLE cpuGetRaiseInterrupt();
extern void cpuSetRaiseInterruptLevel(ULO raise_irq_level);
extern ULO cpuGetRaiseInterruptLevel();

// Exceptions
extern void cpuThrowPrivilegeViolationException();
extern void cpuThrowIllegalInstructionException(BOOLE executejmp);
extern void cpuThrowFLineException();
extern void cpuThrowALineException();
extern void cpuThrowTrapVException();
extern void cpuThrowTrapException(ULO vector_no);
extern void cpuThrowDivisionByZeroException();
extern void cpuThrowChkException();
extern void cpuThrowTraceException();
extern void cpuThrowResetException();
extern void cpuCallResetExceptionFunc();
extern void cpuFrame1(UWO vector_offset, ULO pc);

// Private help functions
constexpr ULO cpuSignExtByteToLong(UBY v)
{
  return (ULO)(LON)(BYT)v;
}
constexpr UWO cpuSignExtByteToWord(UBY v)
{
  return (UWO)(WOR)(BYT)v;
}
constexpr ULO cpuSignExtWordToLong(UWO v)
{
  return (ULO)(LON)(WOR)v;
}
static ULO cpuJoinWordToLong(UWO upper, UWO lower)
{
  return (((ULO)upper) << 16) | ((ULO)lower);
}
static ULO cpuJoinByteToLong(UBY upper, UBY midh, UBY midl, UBY lower)
{
  return (((ULO)upper) << 24) | (((ULO)midh) << 16) | (((ULO)midl) << 8) | ((ULO)lower);
}
static UWO cpuJoinByteToWord(UBY upper, UBY lower)
{
  return (((UWO)upper) << 8) | ((UWO)lower);
}
constexpr BOOLE cpuMsbB(UBY v)
{
  return v >> 7;
}
constexpr BOOLE cpuMsbW(UWO v)
{
  return v >> 15;
}
constexpr BOOLE cpuMsbL(ULO v)
{
  return v >> 31;
}
constexpr BOOLE cpuIsZeroB(UBY v)
{
  return v == 0;
}
constexpr BOOLE cpuIsZeroW(UWO v)
{
  return v == 0;
}
constexpr BOOLE cpuIsZeroL(ULO v)
{
  return v == 0;
}

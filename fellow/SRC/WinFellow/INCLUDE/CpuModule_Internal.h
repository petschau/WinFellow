#pragma once

// This header file defines the internal interfaces of the CPU module.
extern void cpuMakeOpcodeTableForModel();
extern void cpuCreateMulTimeTables();

// StackFrameGen
extern void cpuStackFrameGenerate(uint16_t vector_no, uint32_t pc);
extern void cpuStackFrameInit();

// Registers
extern uint32_t cpu_sr;  // Not static because the flags calculation uses it extensively
extern BOOLE cpuGetFlagSupervisor();
extern BOOLE cpuGetFlagMaster();
extern void cpuSetUspDirect(uint32_t usp);
extern uint32_t cpuGetUspDirect();
extern uint32_t cpuGetUspAutoMap();
extern void cpuSetSspDirect(uint32_t ssp);
extern uint32_t cpuGetSspDirect();
extern uint32_t cpuGetSspAutoMap();
extern void cpuSetMspDirect(uint32_t msp);
extern uint32_t cpuGetMspDirect();
extern uint32_t cpuGetMspAutoMap();
extern void cpuSetMspAutoMap(uint32_t new_msp);
extern uint32_t cpuGetIspAutoMap();
extern void cpuSetIspAutoMap(uint32_t new_isp);
extern void cpuSetDReg(uint32_t i, uint32_t value);
extern uint32_t cpuGetDReg(uint32_t i);
extern void cpuSetAReg(uint32_t i, uint32_t value);
extern uint32_t cpuGetAReg(uint32_t i);
extern void cpuSetReg(uint32_t da, uint32_t i, uint32_t value);
extern uint32_t cpuGetReg(uint32_t da, uint32_t i);
extern void cpuSetPC(uint32_t address);
extern uint32_t cpuGetPC();
extern void cpuSetStop(BOOLE stop);
extern BOOLE cpuGetStop();
extern void cpuSetVbr(uint32_t vbr);
extern uint32_t cpuGetVbr();
extern void cpuSetSfc(uint32_t sfc);
extern uint32_t cpuGetSfc();
extern void cpuSetDfc(uint32_t dfc);
extern uint32_t cpuGetDfc();
extern void cpuSetCacr(uint32_t cacr);
extern uint32_t cpuGetCacr();
extern void cpuSetCaar(uint32_t caar);
extern uint32_t cpuGetCaar();
extern void cpuSetSR(uint32_t sr);
extern uint32_t cpuGetSR();
extern void cpuSetInstructionTime(uint32_t cycles);
extern uint32_t cpuGetInstructionTime();
extern void cpuSetOriginalPC(uint32_t pc);
extern uint32_t cpuGetOriginalPC();
extern void cpuSetInstructionAborted(bool aborted);
extern bool cpuGetInstructionAborted();

#ifdef CPU_INSTRUCTION_LOGGING

extern void cpuSetCurrentOpcode(uint16_t opcode);
extern uint16_t cpuGetCurrentOpcode();

#endif

extern void cpuProfileWrite();

extern void cpuSetModelMask(uint8_t model_mask);
extern uint8_t cpuGetModelMask();
extern void cpuSetDRegWord(uint32_t regno, uint16_t val);
extern void cpuSetDRegByte(uint32_t regno, uint8_t val);
extern uint16_t cpuGetRegWord(uint32_t i, uint32_t regno);
extern uint16_t cpuGetDRegWord(uint32_t regno);
extern uint8_t cpuGetDRegByte(uint32_t regno);
extern uint32_t cpuGetDRegWordSignExtLong(uint32_t regno);
extern uint16_t cpuGetDRegByteSignExtWord(uint32_t regno);
extern uint32_t cpuGetDRegByteSignExtLong(uint32_t regno);
extern uint16_t cpuGetARegWord(uint32_t regno);
extern uint8_t cpuGetARegByte(uint32_t regno);

extern uint16_t cpuGetNextWord();
extern uint32_t cpuGetNextWordSignExt();
extern uint32_t cpuGetNextLong();
extern void cpuSkipNextWord();
extern void cpuSkipNextLong();
extern void cpuClearPrefetch();
extern void cpuValidateReadPointer();

extern void cpuInitializeFromNewPC(uint32_t new_pc);

// Effective address
extern uint32_t cpuEA02(uint32_t regno);
extern uint32_t cpuEA03(uint32_t regno, uint32_t size);
extern uint32_t cpuEA04(uint32_t regno, uint32_t size);
extern uint32_t cpuEA05(uint32_t regno);
extern uint32_t cpuEA06(uint32_t regno);
extern uint32_t cpuEA70();
extern uint32_t cpuEA71();
extern uint32_t cpuEA72();
extern uint32_t cpuEA73();

// Flags
extern void cpuSetFlagsAdd(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsSub(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsCmp(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetZFlagBitOpsB(uint8_t res);
extern void cpuSetZFlagBitOpsL(uint32_t res);

extern void cpuSetFlagsNZ00NewB(uint8_t res);
extern void cpuSetFlagsNZ00NewW(uint16_t res);
extern void cpuSetFlagsNZ00NewL(uint32_t res);
extern void cpuSetFlagsNZ00New64(int64_t res);

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
extern void cpuSetFlagsRotateX(uint16_t z, uint16_t rm, uint16_t x);
extern void cpuSetFlagsAddX(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsSubX(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsAbs(uint16_t f);
extern uint16_t cpuGetZFlagB(uint8_t res);
extern uint16_t cpuGetZFlagW(uint16_t res);
extern uint16_t cpuGetZFlagL(uint32_t res);
extern uint16_t cpuGetNFlagB(uint8_t res);
extern uint16_t cpuGetNFlagW(uint16_t res);
extern uint16_t cpuGetNFlagL(uint32_t res);
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
extern BOOLE cpuCalculateConditionCode(uint32_t cc);

// Logging
#ifdef CPU_INSTRUCTION_LOGGING
extern void cpuCallInstructionLoggingFunc();
extern void cpuCallExceptionLoggingFunc(char *description, uint32_t original_pc, uint16_t opcode);
extern void cpuCallInterruptLoggingFunc(uint32_t level, uint32_t vector_address);
#endif

// Interrupt
extern uint32_t cpuActivateSSP();
extern void cpuSetRaiseInterrupt(BOOLE raise_irq);
extern BOOLE cpuGetRaiseInterrupt();
extern void cpuSetRaiseInterruptLevel(uint32_t raise_irq_level);
extern uint32_t cpuGetRaiseInterruptLevel();

// Exceptions
extern void cpuThrowPrivilegeViolationException();
extern void cpuThrowIllegalInstructionException(BOOLE executejmp);
extern void cpuThrowIllegalInstructionExceptionFromBreakpoint();
extern void cpuThrowFLineException();
extern void cpuThrowALineException();
extern void cpuThrowTrapVException();
extern void cpuThrowTrapException(uint32_t vector_no);
extern void cpuThrowDivisionByZeroException();
extern void cpuThrowChkException();
extern void cpuThrowTraceException();
extern void cpuThrowResetException();
extern void cpuCallResetExceptionFunc();
extern void cpuFrame1(uint16_t vector_offset, uint32_t pc);

// Private help functions
static uint32_t cpuSignExtByteToLong(uint8_t v) {return (uint32_t)(int32_t)(int8_t) v;}
static uint16_t cpuSignExtByteToWord(uint8_t v) {return (uint16_t)(int16_t)(int8_t) v;}
static uint32_t cpuSignExtWordToLong(uint16_t v) {return (uint32_t)(int32_t)(int16_t) v;}
static uint32_t cpuJoinWordToLong(uint16_t upper, uint16_t lower) {return (((uint32_t)upper) << 16) | ((uint32_t)lower);}
static uint32_t cpuJoinByteToLong(uint8_t upper, uint8_t midh, uint8_t midl, uint8_t lower) {return (((uint32_t)upper) << 24) | (((uint32_t)midh) << 16) | (((uint32_t)midl) << 8) | ((uint32_t)lower);}
static uint16_t cpuJoinByteToWord(uint8_t upper, uint8_t lower) {return (((uint16_t)upper) << 8) | ((uint16_t)lower);}
static BOOLE cpuMsbB(uint8_t v) {return v>>7;}
static BOOLE cpuMsbW(uint16_t v) {return v>>15;}
static BOOLE cpuMsbL(uint32_t v) {return v>>31;}
static BOOLE cpuIsZeroB(uint8_t v) {return v == 0;}
static BOOLE cpuIsZeroW(uint16_t v) {return v == 0;}
static BOOLE cpuIsZeroL(uint32_t v) {return v == 0;}

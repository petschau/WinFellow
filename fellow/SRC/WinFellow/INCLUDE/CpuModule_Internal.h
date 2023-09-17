#ifndef CpuModule_Internal_H
#define CpuModule_Internal_H

// This header file defines the internal interfaces of the CPU module.
extern void cpuMakeOpcodeTableForModel(void);
extern void cpuCreateMulTimeTables(void);

// StackFrameGen
extern void cpuStackFrameGenerate(UWO vector_no, uint32_t pc);
extern void cpuStackFrameInit(void);

// Registers
extern uint32_t cpu_sr;  // Not static because the flags calculation uses it extensively
extern BOOLE cpuGetFlagSupervisor(void);
extern BOOLE cpuGetFlagMaster(void);
extern void cpuSetUspDirect(uint32_t usp);
extern uint32_t cpuGetUspDirect(void);
extern uint32_t cpuGetUspAutoMap(void);
extern void cpuSetSspDirect(uint32_t ssp);
extern uint32_t cpuGetSspDirect(void);
extern uint32_t cpuGetSspAutoMap(void);
extern void cpuSetMspDirect(uint32_t msp);
extern uint32_t cpuGetMspDirect(void);
extern uint32_t cpuGetMspAutoMap(void);
extern void cpuSetMspAutoMap(uint32_t new_msp);
extern uint32_t cpuGetIspAutoMap(void);
extern void cpuSetIspAutoMap(uint32_t new_isp);
extern void cpuSetDReg(uint32_t i, uint32_t value);
extern uint32_t cpuGetDReg(uint32_t i);
extern void cpuSetAReg(uint32_t i, uint32_t value);
extern uint32_t cpuGetAReg(uint32_t i);
extern void cpuSetReg(uint32_t da, uint32_t i, uint32_t value);
extern uint32_t cpuGetReg(uint32_t da, uint32_t i);
extern void cpuSetPC(uint32_t address);
extern uint32_t cpuGetPC(void);
extern void cpuSetStop(BOOLE stop);
extern BOOLE cpuGetStop(void);
extern void cpuSetVbr(uint32_t vbr);
extern uint32_t cpuGetVbr(void);
extern void cpuSetSfc(uint32_t sfc);
extern uint32_t cpuGetSfc(void);
extern void cpuSetDfc(uint32_t dfc);
extern uint32_t cpuGetDfc(void);
extern void cpuSetCacr(uint32_t cacr);
extern uint32_t cpuGetCacr(void);
extern void cpuSetCaar(uint32_t caar);
extern uint32_t cpuGetCaar(void);
extern void cpuSetSR(uint32_t sr);
extern uint32_t cpuGetSR(void);
extern void cpuSetInstructionTime(uint32_t cycles);
extern uint32_t cpuGetInstructionTime(void);
extern void cpuSetOriginalPC(uint32_t pc);
extern uint32_t cpuGetOriginalPC(void);
extern void cpuSetInstructionAborted(bool aborted);
extern bool cpuGetInstructionAborted();

#ifdef CPU_INSTRUCTION_LOGGING

extern void cpuSetCurrentOpcode(UWO opcode);
extern UWO cpuGetCurrentOpcode(void);

#endif

extern void cpuProfileWrite(void);

extern void cpuSetModelMask(UBY model_mask);
extern UBY cpuGetModelMask(void);
extern void cpuSetDRegWord(uint32_t regno, UWO val);
extern void cpuSetDRegByte(uint32_t regno, UBY val);
extern UWO cpuGetRegWord(uint32_t i, uint32_t regno);
extern UWO cpuGetDRegWord(uint32_t regno);
extern UBY cpuGetDRegByte(uint32_t regno);
extern uint32_t cpuGetDRegWordSignExtLong(uint32_t regno);
extern UWO cpuGetDRegByteSignExtWord(uint32_t regno);
extern uint32_t cpuGetDRegByteSignExtLong(uint32_t regno);
extern UWO cpuGetARegWord(uint32_t regno);
extern UBY cpuGetARegByte(uint32_t regno);

extern UWO cpuGetNextWord(void);
extern uint32_t cpuGetNextWordSignExt(void);
extern uint32_t cpuGetNextLong(void);
extern void cpuSkipNextWord(void);
extern void cpuSkipNextLong(void);
extern void cpuClearPrefetch(void);
extern void cpuValidateReadPointer(void);

extern void cpuInitializeFromNewPC(uint32_t new_pc);

// Effective address
extern uint32_t cpuEA02(uint32_t regno);
extern uint32_t cpuEA03(uint32_t regno, uint32_t size);
extern uint32_t cpuEA04(uint32_t regno, uint32_t size);
extern uint32_t cpuEA05(uint32_t regno);
extern uint32_t cpuEA06(uint32_t regno);
extern uint32_t cpuEA70(void);
extern uint32_t cpuEA71(void);
extern uint32_t cpuEA72(void);
extern uint32_t cpuEA73(void);

// Flags
extern void cpuSetFlagsAdd(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsSub(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetFlagsCmp(BOOLE z, BOOLE rm, BOOLE dm, BOOLE sm);
extern void cpuSetZFlagBitOpsB(UBY res);
extern void cpuSetZFlagBitOpsL(uint32_t res);

extern void cpuSetFlagsNZ00NewB(UBY res);
extern void cpuSetFlagsNZ00NewW(UWO res);
extern void cpuSetFlagsNZ00NewL(uint32_t res);
extern void cpuSetFlagsNZ00New64(LLO res);

extern void cpuSetFlagZ(BOOLE f);
extern void cpuSetFlagN(BOOLE f);
extern void cpuSetFlagV(BOOLE f);
extern void cpuSetFlagC(BOOLE f);
extern void cpuSetFlagXC(BOOLE f);
extern void cpuSetFlags0100(void);
extern void cpuSetFlagsNeg(BOOLE z, BOOLE rm, BOOLE dm);
extern BOOLE cpuGetFlagX(void);
extern void cpuSetFlagsNegx(BOOLE z, BOOLE rm, BOOLE dm);
extern BOOLE cpuGetFlagV(void);
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
extern UWO cpuGetZFlagL(uint32_t res);
extern UWO cpuGetNFlagB(UBY res);
extern UWO cpuGetNFlagW(UWO res);
extern UWO cpuGetNFlagL(uint32_t res);
extern void cpuClearFlagsVC(void);

extern BOOLE cpuCalculateConditionCode0(void);
extern BOOLE cpuCalculateConditionCode1(void);
extern BOOLE cpuCalculateConditionCode2(void);
extern BOOLE cpuCalculateConditionCode3(void);
extern BOOLE cpuCalculateConditionCode4(void);
extern BOOLE cpuCalculateConditionCode5(void);
extern BOOLE cpuCalculateConditionCode6(void);
extern BOOLE cpuCalculateConditionCode7(void);
extern BOOLE cpuCalculateConditionCode8(void);
extern BOOLE cpuCalculateConditionCode9(void);
extern BOOLE cpuCalculateConditionCode10(void);
extern BOOLE cpuCalculateConditionCode11(void);
extern BOOLE cpuCalculateConditionCode12(void);
extern BOOLE cpuCalculateConditionCode13(void);
extern BOOLE cpuCalculateConditionCode14(void);
extern BOOLE cpuCalculateConditionCode15(void);
extern BOOLE cpuCalculateConditionCode(uint32_t cc);

// Logging
#ifdef CPU_INSTRUCTION_LOGGING
extern void cpuCallInstructionLoggingFunc(void);
extern void cpuCallExceptionLoggingFunc(STR *description, uint32_t original_pc, UWO opcode);
extern void cpuCallInterruptLoggingFunc(uint32_t level, uint32_t vector_address);
#endif

// Interrupt
extern uint32_t cpuActivateSSP(void);
extern void cpuSetRaiseInterrupt(BOOLE raise_irq);
extern BOOLE cpuGetRaiseInterrupt(void);
extern void cpuSetRaiseInterruptLevel(uint32_t raise_irq_level);
extern uint32_t cpuGetRaiseInterruptLevel(void);

// Exceptions
extern void cpuThrowPrivilegeViolationException(void);
extern void cpuThrowIllegalInstructionException(BOOLE executejmp);
extern void cpuThrowIllegalInstructionExceptionFromBreakpoint();
extern void cpuThrowFLineException(void);
extern void cpuThrowALineException(void);
extern void cpuThrowTrapVException(void);
extern void cpuThrowTrapException(uint32_t vector_no);
extern void cpuThrowDivisionByZeroException();
extern void cpuThrowChkException(void);
extern void cpuThrowTraceException(void);
extern void cpuThrowResetException(void);
extern void cpuCallResetExceptionFunc(void);
extern void cpuFrame1(UWO vector_offset, uint32_t pc);

// Private help functions
static uint32_t cpuSignExtByteToLong(UBY v) {return (uint32_t)(LON)(BYT) v;}
static UWO cpuSignExtByteToWord(UBY v) {return (UWO)(WOR)(BYT) v;}
static uint32_t cpuSignExtWordToLong(UWO v) {return (uint32_t)(LON)(WOR) v;}
static uint32_t cpuJoinWordToLong(UWO upper, UWO lower) {return (((uint32_t)upper) << 16) | ((uint32_t)lower);}
static uint32_t cpuJoinByteToLong(UBY upper, UBY midh, UBY midl, UBY lower) {return (((uint32_t)upper) << 24) | (((uint32_t)midh) << 16) | (((uint32_t)midl) << 8) | ((uint32_t)lower);}
static UWO cpuJoinByteToWord(UBY upper, UBY lower) {return (((UWO)upper) << 8) | ((UWO)lower);}
static BOOLE cpuMsbB(UBY v) {return v>>7;}
static BOOLE cpuMsbW(UWO v) {return v>>15;}
static BOOLE cpuMsbL(uint32_t v) {return v>>31;}
static BOOLE cpuIsZeroB(UBY v) {return v == 0;}
static BOOLE cpuIsZeroW(UWO v) {return v == 0;}
static BOOLE cpuIsZeroL(uint32_t v) {return v == 0;}


#endif
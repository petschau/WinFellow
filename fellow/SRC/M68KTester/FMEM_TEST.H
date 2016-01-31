#ifndef FMEM_TEST_H
#define FMEM_TEST_H

// Stubs for memory access in M68KTester

/* Memory access functions */

extern UBY memoryReadByte(ULO address);
extern UWO memoryReadWord(ULO address);
extern ULO memoryReadLong(ULO address);
extern void memoryWriteByte(UBY data, ULO address);
extern void memoryWriteWord(UWO data, ULO address);
extern void memoryWriteLong(ULO data, ULO address);

extern ULO memory_fault_address;
extern BOOLE memory_fault_read;

// Stub for UAE calltrap in M68KTester

#define call_calltrap

#endif

#ifndef MODRIP_LINUX
#define MODRIP_LINUX

BOOLE modripGuiSaveRequest(struct ModuleInfo *info, MemoryAccessFunc func);
BOOLE modripGuiRipMemory(void);
void modripGuiError(char *message);
BOOLE modripGuiRipFloppy(int driveNo);
BOOLE modripGuiDumpChipMem(void);
BOOLE modripGuiRunProWiz(void);
BOOLE modripGuiInitialize(void);
void modripGuiSetBusy(void);
void modripGuiUnSetBusy(void);
void modripGuiUnInitialize(void);

#endif
/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* Code generator for the 68k emulation.                                   */
/*                                                                         */
/* Author: Petter Schau                                                    */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define stricmp _stricmp
#define strnicmp _strnicmp

typedef struct
{
  char instruction_name[32];
  char cpu_model_mask[32];
  char description[32];
  char function[32];
  char format[32];
  char size[32];
  char opcode[32];
  char eamask[32];
  char eamask2[32];
  char eaisdest[32];
  char reg[32];
  char disasm_func_no[10];
} cpu_instruction_info;

typedef struct
{
  char name[32];
  int data[3];
  int dis_func_no;
} cpu_data;

unsigned int cg_ea_time[3][12] =  {{  0,  0,  4,  4,  6,  8, 10,  8, 12,  8, 10,  4},
                                   {  0,  0,  4,  4,  6,  8, 10,  8, 12,  8, 10,  4},
			           {  0,  0,  8,  8, 10, 12, 14, 12, 16, 12, 14,  8}};

unsigned int cg_lea_time[12] =     {  0,  0,  4,  0,  0,  8, 12,  8, 12,  8, 12,  0};
unsigned int cg_pea_time[12] =     {  0,  0, 12,  0,  0, 16, 20, 16, 20, 16, 20,  0};
unsigned int cg_jsr_time[12] =     {  0,  0, 16,  0,  0, 18, 22, 18, 20, 18, 22,  0};
unsigned int cg_jmp_time[12] =     {  0,  0,  8,  0,  0, 10, 14, 10, 12, 10, 14,  0};
unsigned int cg_movemtomem_time[12] = {  0,  0,  8,  0,  0, 12, 14, 12, 16,  0,  0,  0};
unsigned int cg_movemtoreg_time[12] = {  0,  0, 12,  0,  0, 16, 18, 16, 20, 16, 18,  0};

unsigned int cg_ea_memory[12] =    {  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1};
unsigned int cg_ea_direct[12] =    {  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
unsigned int cg_ea_immediate[12] = {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1};

int cpu_profile = 0;

char cpucode_path[512];
char cpudata_path[512];
char cpudisfunc_path[512];
char cpudecl_path[512];
char cpuprofile_path[512];

FILE *codef = NULL;
FILE *dataf = NULL;
FILE *disfuncf = NULL;
FILE *declf = NULL;
FILE *profilef = NULL;

cpu_data cpu_opcode_data[65536];
unsigned char cpu_opcode_model_mask[65536];
cpu_instruction_info cpu_opcode_info[1000];
unsigned int cpuinfo_count;

char cg_profile_names[4000][32];
int cg_profile_count = 0;

#define M68000 0x01
#define M68010 0x02
#define M68020 0x04
#define M68030 0x08
#define M68040 0x10

void cgErrorMsg(char *templ_name)
{
  printf("ERROR: BUG in %s template.\n", templ_name);
}

/*=====================*/
/* Profiling functions */
/*=====================*/

void cgProfileAdd(char *name)
{
  int i;
  for (i = 0; i < cg_profile_count; ++i)
    if (strcmp(name, cg_profile_names[i]) == 0)
      return;
  strcpy(cg_profile_names[i], name);
  cg_profile_count++;
}

void cgProfileIn(char *name)
{
  if (!cpu_profile) return;
  cgProfileAdd(name);
  fprintf(codef, "\tcpuTscBefore(&%s_profile_tmp);\n{\n", name);
}

void cgProfileOut(char *name)
{
  if (!cpu_profile) return;
  fprintf(codef, "}\n\tcpuTscAfter(&%s_profile_tmp, &%s_profile, &%s_profile_times);\n", name, name, name);
}

void cgProfileLogHeader()
{
  if (cpu_profile)
  {
    fprintf(profilef, "#include \"fileops.h\"\n");
  }
  fprintf(profilef, "void cpuProfileWrite(void)\n");
  fprintf(profilef, "{\n");

  if (!cpu_profile)
  {
    return;
  }

  fprintf(profilef, "\tchar filename[MAX_PATH];\n");
  fprintf(profilef, "\tFILE *F = NULL;\n");
  fprintf(profilef, "\tfileopsGetGenericFileName(filename, \"WinFellow\", \"cpuprofile.txt\");\n");
  fprintf(profilef, "\tF = fopen(filename, \"w\");\n");
  fprintf(profilef, "\tif (F != NULL)\n");
  fprintf(profilef, "\t{\n");
  fprintf(profilef, "\t\tfprintf(F, \"NAME\tTOTAL_CYCLES\tCALL_COUNT\tCYCLES_PER_CALL\\n\");\n");
}

void cgProfileLogFooter()
{
  if (!cpu_profile)
  {
    fprintf(profilef, "}\n");
    return;
  }
  fprintf(profilef, "\t\tfclose(F);\n");
  fprintf(profilef, "\t}\n");
  fprintf(profilef, "}\n");
}

void cgProfileDeclare(char *name)
{
  fprintf(dataf, "LLO %s_profile_tmp = 0;\n", name);
  fprintf(dataf, "LLO %s_profile = 0;\n", name);
  fprintf(dataf, "int32_t %s_profile_times = 0;\n", name);
}

void cgProfileLogLine(char *name)
{
  if (!cpu_profile) return;
  fprintf(profilef, "\t\tfprintf(F, \"%s\\t%%I64d\\t%%d\\t%%I64d\\n\", %s_profile, %s_profile_times, (%s_profile_times == 0) ? 0 : (%s_profile / %s_profile_times));\n", name, name, name, name, name, name);
}

void cgProfileDeclarations()
{
  int i;
  if (!cpu_profile) return;
  for (i = 0; i < cg_profile_count; ++i)
    cgProfileDeclare(cg_profile_names[i]);
  for (i = 0; i < cg_profile_count; ++i)
    cgProfileLogLine(cg_profile_names[i]);
}

/*=======================*/
/* Disassembly functions */
/*=======================*/

void cgSetDisassemblyFunction(unsigned int opcode, char *dis_func_no)
{
  cpu_opcode_data[opcode].dis_func_no = atoi(dis_func_no);
}

void cgDisFunc(void)
{
  unsigned int opcode;
  fprintf(disfuncf, "static uint8_t cpu_dis_func_tab[65536] = \n{");
  for (opcode = 0; opcode < 65536; opcode += 16)
  {
    fprintf(disfuncf,
	    "%s\n%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
	    (opcode != 0) ? ",":"",
	    cpu_opcode_data[opcode].dis_func_no,
	    cpu_opcode_data[opcode+1].dis_func_no,
	    cpu_opcode_data[opcode+2].dis_func_no,
	    cpu_opcode_data[opcode+3].dis_func_no,
	    cpu_opcode_data[opcode+4].dis_func_no,
	    cpu_opcode_data[opcode+5].dis_func_no,
	    cpu_opcode_data[opcode+6].dis_func_no,
	    cpu_opcode_data[opcode+7].dis_func_no,
	    cpu_opcode_data[opcode+8].dis_func_no,
	    cpu_opcode_data[opcode+9].dis_func_no,
	    cpu_opcode_data[opcode+10].dis_func_no,
	    cpu_opcode_data[opcode+11].dis_func_no,
	    cpu_opcode_data[opcode+12].dis_func_no,
	    cpu_opcode_data[opcode+13].dis_func_no,
	    cpu_opcode_data[opcode+14].dis_func_no,
	    cpu_opcode_data[opcode+15].dis_func_no);
  }
  fprintf(disfuncf, "\n};\n");  
}

/*=======================*/
/* Instruction functions */
/*=======================*/

void cgMakeFunctionName(char *fname, char *name, unsigned int opcode)
{
  sprintf(fname, "%s_%.4X", name, opcode);
}

void cgDeclareFunction(char *fname)
{
  fprintf(declf, "static void %s(uint32_t*opc_data);\n", fname);
}

void cgMakeFunctionHeader(char *fname, char *templ_name)
{
  fprintf(codef, "static void %s(uint32_t*opc_data)\n", fname);
  fprintf(codef, "{\n");
  //cgProfileIn(templ_name);
  cgProfileIn(fname);
}

void cgMakeFunctionFooter(char *fname, char *templ_name)
{
//  cgProfileOut(templ_name);
  cgProfileOut(fname);
  fprintf(codef, "}\n");
}

void cgMakeInstructionTime(unsigned int time_cpu_data_index)
{
  fprintf(codef, "\tcpuSetInstructionTime(opc_data[%d]);\n", time_cpu_data_index);
}

void cgMakeInstructionTimeAbs(unsigned int cycles)
{
  fprintf(codef, "\tcpuSetInstructionTime(%d);\n", cycles);
}

void cgMakeInstructionTimeAbsExtraAdd(unsigned int cycles, char *extra_add)
{
  fprintf(codef, "\tcpuSetInstructionTime(%d%s);\n", cycles, extra_add);
}

void cgCopyFunctionName(char *fname, char *name, char *templ_name)
{
  if (strcmp(name, "cpuIllegalInstruction") != 0) cgErrorMsg(templ_name);
  strcpy(name, fname);
}

void cgSetData(unsigned int opcode, unsigned int index, unsigned int data)
{
  cpu_opcode_data[opcode].data[index] = data;
}

void cgSetModelMask(unsigned int opcode, char *model_mask)
{
  unsigned char mask = 0;
  if (model_mask[0] == '1') mask |= M68000;
  if (model_mask[1] == '1') mask |= M68010;
  if (model_mask[2] == '1') mask |= M68020;
  if (model_mask[3] == '1') mask |= M68030;
  if (model_mask[4] == '1') mask |= M68040;
  cpu_opcode_model_mask[opcode] = mask;
}

unsigned int cgGetEaNo(unsigned int eano, unsigned int eareg)
{
  return (eano < 7) ? eano : (eano + eareg);
}

unsigned int cgEAReg(unsigned int eano, unsigned int eareg)
{
  if (eano < 7) return (eano << 3) | eareg;
  else return (7 << 3) | (eano - 7);
}

unsigned int cgEAReg2(unsigned int eano, unsigned int eareg)
{
  if (eano < 7) return (eano << 6) | (eareg << 9);
  else return (7 << 6) | ((eano - 7) << 9);
}

char *cgSize(unsigned int size)
{
  if (size == 1) return "uint8_t";
  else if (size == 2) return "uint16_t";
  else if (size == 4) return "uint32_t";
  return "ERROR_cgSize()";
}

char *cgCastSize(unsigned int size)
{
  if (size == 1) return "(uint8_t)";
  else if (size == 2) return "(uint16_t)";
  else if (size == 4) return "(uint32_t)";
  return "ERROR_cgCastSize()";
}

char *cgMemoryFetch(unsigned int size)
{
  if (size == 1) return "memoryReadByte";
  else if (size == 2) return "memoryReadWord";
  else if (size == 4) return "memoryReadLong";
  return "ERROR_cgMemoryFetch()";
}

char *cgMemoryStore(unsigned int size)
{
  if (size == 1) return "memoryWriteByte";
  else if (size == 2) return "memoryWriteWord";
  else if (size == 4) return "memoryWriteLong";
  return "ERROR_cgMemoryStore()";
}

char *cgCalculateEA(unsigned int eano)
{
  if (eano == 2) return "cpuEA02";
  else if (eano == 3) return "cpuEA03";
  else if (eano == 4) return "cpuEA04";
  else if (eano == 5) return "cpuEA05";
  else if (eano == 6) return "cpuEA06";
  else if (eano == 7) return "cpuEA70";
  else if (eano == 8) return "cpuEA71";
  else if (eano == 9) return "cpuEA72";
  else if (eano == 10) return "cpuEA73";
  return "ERROR_cgCalculateEA()";
}

unsigned int cgGetSizeCycleIndex(unsigned int size)
{
  if (size == 1) return 0;
  else if (size == 2) return 1;
  else return 2;
}

// Generate the source value fetched from a mode register pair.
// Throw away the actual ea address.
void cgFetchSrc(unsigned int eano, unsigned int eareg_cpu_data_index, unsigned int size, char regtype, unsigned int reg_cpu_data_index)
{
  // Declare source tmp variable
  if (regtype == 'A')
  {
    // If the regtype is 'A', this value will be used with an operation on an address register. (Always 32-bit)
    // The source value is read from the specified ea mode below.
    // Declare 32-bit and set up sign extension from the specified size.
    // The source size will never be a byte.
    fprintf(codef, "\tuint32_t src = %s%s", (size == 4) ? "" : "(uint32_t)(int32_t)", (size == 2) ? "(int16_t)" : "");
    if (size == 1) fprintf(codef, "BUG! cgFetchSrc(), byte size used with regtype 'A'");
  }
  else if (regtype == 'B')
  {
    // The source value is a Quick constant, to be used on an address register. (Always 32-bit)
    fprintf(codef, "\tuint32_t src = opc_data[%d];\n", reg_cpu_data_index);
    return;
  }
  else if (regtype == 'Q')
  {
    // The source value is a Quick constant, to be used according to the specified size.
    fprintf(codef, "\t%s src = %sopc_data[%d];\n", cgSize(size), (size != 4) ? cgCastSize(size) : "", reg_cpu_data_index);
    return;
  }
  else
  {
    // The source value is read from the specified ea mode below.
    fprintf(codef, "\t%s src = ", cgSize(size));
  }

  // Create fetch statement to read the source value.
  if (regtype == 'I' || cg_ea_immediate[eano])
  {
    // Read source value from an immediate word.
    fprintf(codef, "%scpuGetNext%s();\n", (size == 1) ? "(uint8_t)" : "", (size <= 2) ? "Word" : "Long");
  }
  else if (regtype == '2')
  {
    // Read source value from an immediate word. Bit number for BCHG etc.
    fprintf(codef, "%scpuGetNextWord();\n", cgCastSize(size));
  }
  else if (eano == 1)
  {
    fprintf(codef, "cpuGetAReg%s(opc_data[%d]);\n", (size == 1) ? "Byte" : ((size == 2) ? "Word" : ""), eareg_cpu_data_index);
  }
  else if (eano == 0)
  {
    fprintf(codef, "cpuGetDReg%s(opc_data[%d]);\n", (size == 1) ? "Byte" : ((size == 2) ? "Word" : ""), eareg_cpu_data_index);
  }
  else if (eano >= 3 && eano <= 4)
  {
    // Read source value from memory, pre or post increment mode.
    fprintf(codef, "%s(%s(opc_data[%d],%d));\n", cgMemoryFetch(size), cgCalculateEA(eano), eareg_cpu_data_index, size);
  }
  else if (eano == 2 || eano == 5 || eano == 6)
  {
    // Read source value from memory.
    fprintf(codef, "%s(%s(opc_data[%d]));\n", cgMemoryFetch(size), cgCalculateEA(eano), eareg_cpu_data_index);
  }
  else if (eano >= 7 && eano < 11)
  {
    // Read source value from memory.
    fprintf(codef, "%s(%s());\n", cgMemoryFetch(size), cgCalculateEA(eano));
  }
  else
  {
    fprintf(codef, "ERROR_cgFetchSrc()");
  }
}

// Generate destination ea and save it.
void cgFetchDstEa(unsigned int eano, unsigned int eareg_cpu_data_index, unsigned int size)
{
  // For register access, do nothing.
  // For memory operands, save ea.
  if (eano >= 3 && eano <= 4)
  {
    // Save destination ea, pre or post increment mode.
    fprintf(codef, "\tuint32_t dstea = %s(opc_data[%d], %d);\n", cgCalculateEA(eano), eareg_cpu_data_index, size);
  }
  else if (eano == 2 || eano == 5 || eano == 6)
  {
    // Save destination ea.
    fprintf(codef, "\tuint32_t dstea = %s(opc_data[%d]);\n", cgCalculateEA(eano), eareg_cpu_data_index);
  }
  else if (eano >= 7 && eano < 11)
  {
    // Save destination ea.
    fprintf(codef, "\tuint32_t dstea = %s();\n", cgCalculateEA(eano));
  }
}

void cgFetchDst(unsigned int eano, unsigned int eareg_cpu_data_index, unsigned int size)
{
  cgFetchDstEa(eano, eareg_cpu_data_index, size);

  // Declare dst value and a cast, for address register, use uint32_t for all.
  fprintf(codef, "\t%s dst = ", (eano == 1) ? "uint32_t" : cgSize(size));

  // Fetch operand
  if (cg_ea_immediate[eano])
  {
    fprintf(codef, "%scpuGetNext%s();\n", (size == 1) ? "(uint8_t)" : "", (size <= 2) ? "Word" : "Long");
  }
  else if (eano == 1)
  {
    fprintf(codef, "cpuGetAReg(opc_data[%d]);\n", eareg_cpu_data_index);
  }
  else if (eano == 0)
  {
    fprintf(codef, "cpuGetDReg%s(opc_data[%d]);\n", (size == 1) ? "Byte" : ((size == 2) ? "Word" : ""), eareg_cpu_data_index);
  }
  else if (eano >= 2 && eano < 11)
  {
    fprintf(codef, "%s(dstea);\n", cgMemoryFetch(size));
  }
  else
  {
    fprintf(codef, "ERROR_cgFetchDst()");
  }
}

// Save destination value to the destination ea. Assumes dstea has been set.
void cgStoreDst(unsigned int eano, unsigned int eareg_cpu_data_index, unsigned int size, char *dstname)
{
  if (eano == 0 && size == 1)
  {
    fprintf(codef, "\tcpuSetDRegByte(opc_data[%d], %s);\n", eareg_cpu_data_index, dstname);
  }
  else if (eano == 0 && size == 2)
  {
    fprintf(codef, "\tcpuSetDRegWord(opc_data[%d], %s);\n", eareg_cpu_data_index, dstname);
  }
  else if (eano == 0 && size == 4)
  {
    fprintf(codef, "\tcpuSetDReg(opc_data[%d], %s);\n", eareg_cpu_data_index, dstname);
  }
  else if (eano == 1)
  {
    fprintf(codef, "\tcpuSetAReg(opc_data[%d], %s);\n", eareg_cpu_data_index, dstname);
  }
  else if (eano >= 2 && eano < 9)
  {
    fprintf(codef, "\t%s(%s, dstea);\n", cgMemoryStore(size), dstname);
  }
  else
  {
    fprintf(codef, "ERROR_cgStoreDst()");
  }
}

int totalcount = 0;

unsigned int cgGetAddqSubqTime(unsigned int size, unsigned int eano)
{
  unsigned int cycles;
  if (eano == 1)
  {
    // Address register is always 32-bit
    cycles = 8;
  }
  else
  {
    if (size <= 2)
    {
      if (eano == 0) cycles = 4;
      else cycles = 8;
    }
    else
    {
      if (eano == 0) cycles = 8;
      else cycles = 12;
    }	
  }
  return cycles + cg_ea_time[cgGetSizeCycleIndex(size)][eano];
}

unsigned int cgGetAddaSubaTime(unsigned int size, unsigned int eano)
{
  unsigned int cycles;
  if (size <= 2)
  {
    // Address register is always 32-bit
    cycles = 8;
  }
  else
  {
    cycles = 6;
    if (cg_ea_direct[eano] || cg_ea_immediate[eano])
    {
      // Increase to 8 if ea is direct or immediate
      cycles += 2;
    }
  }
  return cycles + cg_ea_time[cgGetSizeCycleIndex(size)][eano];
}

unsigned int cgGetAddSubOrAndTime(unsigned int size, unsigned int eano, int ea_is_dst)
{
  unsigned int cycles = 4 + cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  if (size <= 2)
  {
    if (ea_is_dst && cg_ea_memory[eano]) cycles += 4;
  }
  else
  {
    // Size is long
    if (!ea_is_dst)
    {
      cycles += 2;
      if (cg_ea_direct[eano] || cg_ea_immediate[eano]) cycles += 2;
    }
    else
    {
      if (cg_ea_memory[eano]) cycles += 8;
    }
  }
  return cycles;
}

unsigned int cgGetAddiSubiOriEoriTime(unsigned int size, unsigned int eano, char regtype)
{
  unsigned int cycles;
  if (size <= 2)
  {
    if (eano <= 1) cycles = 8;
    else cycles = 12;
  }
  else
  {
    if (eano <= 1) cycles = 16;
    else cycles = 20;
  }
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetCmpiTime(unsigned int size, unsigned int eano)
{
  unsigned int cycles;
  if (size <= 2)
  {
    cycles = 8;
  }
  else
  {
    if (eano <= 1) cycles = 14;
    else cycles = 12;
  }
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetAndiTime(unsigned int size, unsigned int eano, char regtype)
{
  unsigned int cycles = 0;
  if (size <= 2)
  {
    if (eano <= 1) cycles = 8;
    else cycles = 12;
  }
  else
  {
    if (eano <= 1) cycles = 14;
    else cycles = 20;
  }
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetEorTime(unsigned int size, unsigned int eano, int ea_is_dst)
{
  unsigned int cycles = 0;
  if (size <= 2)
  {
    cycles = 4;
    if (ea_is_dst && cg_ea_memory[eano]) cycles += 4;
  }
  else 
  {
    cycles = 8;
    if (ea_is_dst && cg_ea_memory[eano]) cycles += 4;
  }
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetCmpaTime(unsigned int size, unsigned int eano)
{
  unsigned int cycles = 6;
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetCmpTime(unsigned int size, unsigned int eano)
{
  unsigned int cycles = 0;
  if (size <= 2) cycles = 4;
  else cycles = 6;
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetChkTime(unsigned int size, unsigned int eano)
{
  unsigned int cycles = 10;
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetBsetBchgTime(unsigned int size, unsigned int eano, char regtype)
{
  unsigned int cycles = 0;

  // There is also a run-time addition of 2 cycles if bit-no >= 16 and the bit-no is in a register (Possible only for register destinations)
  if (regtype == 'D')
  {
    // Dynamic includes time to write byte back to memory, if using a memory ea (byte)
    if (eano >= 2) cycles = 8;
    else cycles = 6;
  }
  else
  {
    // Static includes time to read additional immediate word, and to write byte back to memory, if using a memory ea (byte)
    if (eano >= 2) cycles = 12;
    else cycles = 10;
  }

  // Memory ea only used for byte size.
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetBclrTime(unsigned int size, unsigned int eano, char regtype)
{
  unsigned int cycles = 0;

  // There is also a run-time addition of 2 cycles if bit-no >= 16 and the bit-no is in a register (Possible only for register destinations)
  if (regtype == 'D')
  {
    // Dynamic includes time to write byte back to memory, if using a memory ea (byte)
    cycles = 8;
  }
  else
  {
    // Static includes time to read additional immediate word, and to write byte back to memory, if using a memory ea (byte)
    cycles = 12;
  }

  // Memory ea only used for byte size.
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetBtstTime(unsigned int size, unsigned int eano, char regtype)
{
  unsigned int cycles = 0;
  if (size <= 2)
  {
    if (regtype == 'D') cycles = 4;
    else cycles = 8;
  }
  else
  {
    if (regtype == 'D') cycles = 6;
    else cycles = 10;
  }
  cycles += cg_ea_time[cgGetSizeCycleIndex(size)][eano];
  return cycles;
}

unsigned int cgGetLeaTime(unsigned int eano)
{
  return cg_lea_time[eano];
}

// ADD ADDA ADDI ADDQ
// SUB SUBA SUBI SUBQ
// AND ANDI
// EOR EORI
// OR  ORI
// CMP CMPA
// CMPI (pc relative adressing, only on 010 and up)
// BCHG BCLR BSET BTST
// LEA
// MULS MULU
// DIVS DIVU DIVL
unsigned int cgAdd(cpu_data *cpudata, cpu_instruction_info i)
{
  char *templ_name = "add";
  char *endchar;
  unsigned int base_opcode = strtoul(i.opcode, &endchar, 0);
  unsigned int eareg_cpu_data_index = 0;
  unsigned int reg_cpu_data_index = 1;
  unsigned int time_cpu_data_index = 2;
  unsigned int size = atoi(i.size);
  int ea_is_dst = i.eaisdest[0] == '1';
  int eano, reg, eareg;
  unsigned int icount = 0;
  char regtype = i.reg[0];

  if (regtype == 'G')
  {
    // operations on CCR/SR
    unsigned int opcode = base_opcode;
    char fname[32];
    icount++;
    cgMakeFunctionName(fname, i.instruction_name, opcode);
    cgDeclareFunction(fname);
    cgMakeFunctionHeader(fname, templ_name);
    fprintf(codef, "\t%s();\n", i.function);
    cgMakeFunctionFooter(fname, templ_name);
    cgCopyFunctionName(fname, cpudata[opcode].name, templ_name);
    cgSetModelMask(opcode, i.cpu_model_mask);
    cgSetDisassemblyFunction(opcode, i.disasm_func_no);
    totalcount++;
  }
  else
  {
    // For each EA in i.eamask
    for (eano = 0; eano < 12; ++eano)
    {
      if (i.eamask[eano] == '1')
      {
	unsigned int opcode = base_opcode | cgEAReg(eano, 0);
	char fname[32];
	icount++;
	cgMakeFunctionName(fname, i.instruction_name, opcode);
	cgDeclareFunction(fname);
	cgMakeFunctionHeader(fname, templ_name);

	if (!ea_is_dst)
	{
	  if (stricmp(i.instruction_name, "LEA") == 0)
	  {
	    cgFetchDstEa(eano, eareg_cpu_data_index, size);
	  }
	  else
	  {
	    if (stricmp(i.instruction_name, "DIVL") == 0) fprintf(codef, "\uint16_t ext = cpuGetNextWord();\n");
	    cgFetchSrc(eano, eareg_cpu_data_index, size, regtype, reg_cpu_data_index);
	    if (stricmp(i.instruction_name, "DIVL") != 0) cgFetchDst((regtype=='A') ? 1:0, reg_cpu_data_index, (regtype == 'V') ? 4 : size);
	  }
 	}
	else
	{
	  cgFetchSrc(0, reg_cpu_data_index, size, regtype, reg_cpu_data_index);
	  cgFetchDst(eano, eareg_cpu_data_index, size);
	}

	if (stricmp(i.instruction_name, "LEA") == 0)
	{
	  fprintf(codef, "\tcpuSetAReg(opc_data[%d], dstea);\n", reg_cpu_data_index);
	}
	else if (stricmp(i.instruction_name, "CHK") == 0)
	{
	  fprintf(codef, "\t%s(dst, src, %u);\n", i.function, cgGetChkTime(size, eano));
	}
	else if (regtype == 'C' || 
	         (stricmp(i.instruction_name, "CMPA") == 0) || 
		 (stricmp(i.instruction_name, "CMPI") == 0) || 
		 (stricmp(i.instruction_name, "BTST") == 0)) 
	{
	  // This instruction does not produce a result value.
	  fprintf(codef, "\t%s(dst, src);\n", i.function);
	}
	else if (regtype == 'M' || regtype == 'V')
	{
	  if (stricmp(i.instruction_name, "DIVL") == 0)
	  {
	    fprintf(codef, "\t%s(src, ext, opc_data[2]);\n", i.function);
	  }
	  else if ((stricmp(i.instruction_name, "MULU") == 0) ||
		   (stricmp(i.instruction_name, "MULS") == 0))
	  {
	    fprintf(codef, "\tuint32_t res = %s(dst, src, %d);\n", i.function, cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
	    cgStoreDst(0, reg_cpu_data_index, 4, "res");
	  }
	  else
	  {
	    fprintf(codef, "\t%s(dst, src, opc_data[1], opc_data[2]);\n", i.function);
	  }
	}
	else
	{
	  fprintf(codef, "\tdst = %s(dst, src);\n", i.function);
	  cgStoreDst((ea_is_dst) ? eano : ((regtype == 'A') ? 1 : 0), (ea_is_dst) ? eareg_cpu_data_index : reg_cpu_data_index, size, "dst");
	}

	if ((stricmp(i.instruction_name, "ADDQ") == 0) ||
	    (stricmp(i.instruction_name, "SUBQ") == 0))
	{
	  cgMakeInstructionTimeAbs(cgGetAddqSubqTime(size, eano));
	}
	else if ((stricmp(i.instruction_name, "ADDA") == 0) ||
		 (stricmp(i.instruction_name, "SUBA") == 0))
	{
	  cgMakeInstructionTimeAbs(cgGetAddaSubaTime(size, eano));
	}
	else if ((stricmp(i.instruction_name, "ADD") == 0) ||
		 (stricmp(i.instruction_name, "SUB") == 0) ||
		 (stricmp(i.instruction_name, "OR") == 0) ||
		 (stricmp(i.instruction_name, "AND") == 0))
	{
	  cgMakeInstructionTimeAbs(cgGetAddSubOrAndTime(size, eano, ea_is_dst));
	}
	else if (stricmp(i.instruction_name, "EOR") == 0)
	{
	  cgMakeInstructionTimeAbs(cgGetEorTime(size, eano, ea_is_dst));
	}
	else if (stricmp(i.instruction_name, "CMPA") == 0)
	{
	  cgMakeInstructionTimeAbs(cgGetCmpaTime(size, eano));
	}
	else if (stricmp(i.instruction_name, "CMP") == 0)
	{
	  cgMakeInstructionTimeAbs(cgGetCmpTime(size, eano));
	}
	else if ((stricmp(i.instruction_name, "ADDI") == 0) ||
		 (stricmp(i.instruction_name, "SUBI") == 0) ||
		 (stricmp(i.instruction_name, "ORI") == 0) ||
		 (stricmp(i.instruction_name, "EORI") == 0))
	{
          // SR/CCR handled elsewhere
	  cgMakeInstructionTimeAbs(cgGetAddiSubiOriEoriTime(size, eano, regtype));
	}
	else if (stricmp(i.instruction_name, "CMPI") == 0)
	{
	  cgMakeInstructionTimeAbs(cgGetCmpiTime(size, eano));
	}
	else if (stricmp(i.instruction_name, "ANDI") == 0)
	{
          // SR/CCR handled elsewhere
          cgMakeInstructionTimeAbs(cgGetAndiTime(size, eano, regtype));
	}
	else if ((stricmp(i.instruction_name, "BSET") == 0) ||
		 (stricmp(i.instruction_name, "BCHG") == 0))
	{
	  if (size == 4 && regtype == 'D')
	  {
	    cgMakeInstructionTimeAbsExtraAdd(cgGetBsetBchgTime(size, eano, regtype), " + ((src >= 16) ? 2 : 0)");
	  }
	  else
	  {
	    cgMakeInstructionTimeAbs(cgGetBsetBchgTime(size, eano, regtype));
	  }
	}
	else if (stricmp(i.instruction_name, "BCLR") == 0)
	{
	  if (size == 4 && regtype == 'D')
	  {
	    cgMakeInstructionTimeAbsExtraAdd(cgGetBclrTime(size, eano, regtype), " + ((src >= 16) ? 2 : 0)");
	  }
	  else
	  {
	    cgMakeInstructionTimeAbs(cgGetBclrTime(size, eano, regtype));
	  }
	}
	else if (stricmp(i.instruction_name, "BTST") == 0)
	{
	  cgMakeInstructionTimeAbs(cgGetBtstTime(size, eano, regtype));
	}
	else if (stricmp(i.instruction_name, "LEA") == 0)
	{
	  cgMakeInstructionTimeAbs(cgGetLeaTime(eano));
	}
	cgMakeFunctionFooter(fname, templ_name);

	// Cycle the registers to set function ptr, data/reg and eareg for each opcode.
	for (reg = 0; (regtype != '2' && regtype != 'I' && (stricmp(i.instruction_name, "DIVL") != 0) && reg < 8) || ((regtype == '2' || regtype == 'I' || (stricmp(i.instruction_name, "DIVL") == 0)) && reg == 0); ++reg)
	{
	  for (eareg = 0; (eano < 7 && eareg < 8) || (eano >= 7 && eareg == 0); ++eareg)
	  {
	    unsigned int cycles = 0;
	    unsigned int opcode2 = opcode | (reg << 9) | eareg;
	    cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	    cgSetData(opcode2, eareg_cpu_data_index, (eano <7) ? eareg : 0);
	    cgSetData(opcode2, reg_cpu_data_index, (regtype == 'Q' || regtype == 'B') ? ((reg == 0) ? 8 : reg) : reg);
	    if (stricmp(i.instruction_name, "DIVS") == 0)
	    {
	      cycles = 150;
	      cycles += cg_ea_time[cgGetSizeCycleIndex(size)][cgGetEaNo(eano, eareg)];
	    }
	    else if (stricmp(i.instruction_name, "DIVU") == 0)
	    {
	      cycles = 132;
	      cycles += cg_ea_time[cgGetSizeCycleIndex(size)][cgGetEaNo(eano, eareg)];
	    }
	    else if (stricmp(i.instruction_name, "DIVL") == 0)
	    {
	      cycles = 4;
	      cycles += cg_ea_time[cgGetSizeCycleIndex(size)][cgGetEaNo(eano, eareg)];
	    }
	    cgSetData(opcode2, time_cpu_data_index, cycles);
	    cgSetModelMask(opcode2, i.cpu_model_mask);
	    cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
	    totalcount++;
	  }
	}
      }
    }
  }
  return icount;
}

unsigned int move_time[3][12][9] = {
  { // Byte
  { 4, 4, 8, 8, 8,12,14,12,16},
  { 4, 4, 8, 8, 8,12,14,12,16},
  { 8, 8,12,12,12,16,18,16,20},
  
  { 8, 8,12,12,12,16,18,16,20},
  {10,10,14,14,14,18,20,18,22},
  {12,12,16,16,16,20,22,20,24},
  
  {14,14,18,18,18,22,24,22,26},
  {12,12,16,16,16,20,22,20,24},
  {16,16,20,20,20,24,26,24,28},
  
  {12,12,16,16,16,20,22,20,24},
  {14,14,18,18,18,22,24,22,26},
  { 8, 8,12,12,12,16,18,16,20}
  },
  { // Word
  { 4, 4, 8, 8, 8,12,14,12,16},
  { 4, 4, 8, 8, 8,12,14,12,16},
  { 8, 8,12,12,12,16,18,16,20},
  
  { 8, 8,12,12,12,16,18,16,20},
  {10,10,14,14,14,18,20,18,22},
  {12,12,16,16,16,20,22,20,24},
  
  {14,14,18,18,18,22,24,22,26},
  {12,12,16,16,16,20,22,20,24},
  {16,16,20,20,20,24,26,24,28},
  
  {12,12,16,16,16,20,22,20,24},
  {14,14,18,18,18,22,24,22,26},
  { 8, 8,12,12,12,16,18,16,20}
  },
  { // Long
  { 4, 4,12,12,12,16,18,16,20},
  { 4, 4,12,12,12,16,18,16,20},
  {12,12,20,20,20,24,26,24,28},
  
  {12,12,20,20,20,24,26,24,28},
  {14,14,22,22,22,26,28,26,30},
  {16,16,24,24,24,28,30,28,32},
  
  {18,18,26,26,26,30,32,30,34},
  {16,16,24,24,24,28,30,28,32},
  {20,20,28,28,28,32,34,32,36},
  
  {16,16,24,24,24,28,30,28,32},
  {18,18,26,26,26,30,32,30,34},
  {12,12,20,20,20,24,26,24,28}
  }
};

unsigned int cgGetMoveTime(unsigned int size, unsigned int srceano, unsigned int dsteano)
{
  unsigned int time = move_time[cgGetSizeCycleIndex(size)][srceano][dsteano];
  return time;
}

// MOVE
// MOVEA
unsigned int cgMove(cpu_data *cpudata, cpu_instruction_info i)
{
  char *templ_name = "move";
  char *endchar;
  unsigned int base_opcode = strtoul(i.opcode, &endchar, 0);
  unsigned int srcreg_cpu_data_index = 0;
  unsigned int dstreg_cpu_data_index = 1;
  unsigned int time_cpu_data_index = 2;
  unsigned int size = atoi(i.size);
  int srceano, srceareg, dsteano, dsteareg;
  unsigned int icount = 0;

  // For each DSTEA in i.eamask2
  for (dsteano = 0; dsteano < 12; ++dsteano)
  {
    if (i.eamask2[dsteano] == '1')
    {
      // For each SRCEA in i.eamask
      for (srceano = 0; srceano < 12; ++srceano)
      {
	if (i.eamask[srceano] == '1')
	{
	  unsigned int opcode = base_opcode | cgEAReg2(dsteano, 0) |cgEAReg(srceano, 0);
	  char fname[32];
	  icount++;
	  cgMakeFunctionName(fname, i.instruction_name, opcode);
	  cgDeclareFunction(fname);
	  cgMakeFunctionHeader(fname, templ_name);

	  cgFetchSrc(srceano, srcreg_cpu_data_index, size, 'D', dstreg_cpu_data_index);
	  if (i.reg[0] == 'D')
	  {
	    cgFetchDstEa(dsteano, dstreg_cpu_data_index, size);
	    fprintf(codef, "\t%s(src);\n", i.function);
	    cgStoreDst(dsteano, dstreg_cpu_data_index, size, "src");
	  }
	  else
	  {
	    if (size == 2) fprintf(codef, "\tcpuSetAReg(opc_data[%d], (uint32_t)(int32_t)(int16_t)src);\n", dstreg_cpu_data_index);
	    else fprintf(codef, "\tcpuSetAReg(opc_data[%d], src);\n", dstreg_cpu_data_index);
	  }
	  cgMakeInstructionTimeAbs(cgGetMoveTime(size, srceano, dsteano));
	  cgMakeFunctionFooter(fname, templ_name);

	  // Cycle the registers to set function ptr, data/reg and eareg for each opcode.
	  for (dsteareg = 0; (dsteano < 7 && dsteareg < 8) || (dsteano >= 7 && dsteareg == 0); ++dsteareg)
	  {
	    for (srceareg = 0; (srceano < 7 && srceareg < 8) || (srceano >= 7 && srceareg == 0); ++srceareg)
	    {
	      unsigned int opcode2 = opcode | (dsteareg << 9) | srceareg;
	      cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	      cgSetData(opcode2, srcreg_cpu_data_index, (srceano <7) ? srceareg : 0);
	      cgSetData(opcode2, dstreg_cpu_data_index, (dsteano <7) ? dsteareg : 0);
	      cgSetModelMask(opcode2, i.cpu_model_mask);
	      cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
	      totalcount++;
	    }
	  }
	}
      }
    }
  }
  return icount;
}

// CLR
// JMP
// JSR
// MOVE CCR  (from CCR is 010 and up)
// MOVE SR   (from SR is privileged on 010 and up)
// NBCD
// NEG
// NEGX
// NOT
// PEA
// Scc
// TAS
// TST (pc relative and imm, only 020 and up, Direct Areg valid only on 020 and up and only word and long)
// BIT field instructions
// CALLM
// MULus.L
unsigned int cgClr(cpu_data *cpudata, cpu_instruction_info i)
{
  char *templ_name = "clr";
  char *endchar;
  unsigned int base_opcode = strtoul(i.opcode, &endchar, 0);
  unsigned int eareg_cpu_data_index = 0;
  unsigned int eatime_cpu_data_index = 1;
  unsigned int cc_cpu_data_index = 1;
  unsigned int size = atoi(i.size);
  int eano, eareg;
  unsigned int icount = 0;
  char regtype = i.reg[0];
  unsigned int cc;

  // For each EA in i.eamask
  for (eano = 0; eano < 12; ++eano)
  {
    if (i.eamask[eano] == '1')
    {
      unsigned int opcode = base_opcode | cgEAReg(eano, 0);
      char fname[32];
      icount++;

      cgMakeFunctionName(fname, i.instruction_name, opcode);
      cgDeclareFunction(fname);
      cgMakeFunctionHeader(fname, templ_name);

      if (stricmp(i.instruction_name, "MOVEM") == 0)
      {
	fprintf(codef, "\uint16_t regs = cpuGetNextWord();\n");
      }
      else if ((stricmp(i.instruction_name, "MULL") == 0)
	       || (stricmp(i.instruction_name, "MOVES") == 0)
	       || (stricmp(i.instruction_name, "CAS") == 0)
	       || (stricmp(i.instruction_name, "CHKCMP2") == 0)
	       || (stricmp(i.instruction_name, "CALLM") == 0)
	       || (stricmp(i.instruction_name, "PFLUSH030") == 0)
	       || (strnicmp(i.instruction_name, "BF", 2) == 0))
      {
	// Read extension word _before_ ea calculation.
	  fprintf(codef, "\uint16_t ext = cpuGetNextWord();\n");
      }

      // Generate fetch
      if (regtype == 'G')
      {
	// This is either move to SR or move to CCR, move to SR is privileged and must be checked before reading ea
	if (stricmp(i.instruction_name, "MOVETOSR") == 0)
	{
	  fprintf(codef, "\tif (!cpuGetFlagSupervisor())\n");
	  fprintf(codef, "\t{\n");
	  fprintf(codef, "\t\tcpuThrowPrivilegeViolationException();\n");
	  fprintf(codef, "\t\treturn;\n");
	  fprintf(codef, "\t}\n");
	}

	cgFetchSrc(eano, eareg_cpu_data_index, size, 'D', 0);
      }
      else if (regtype == '0')
      {
	fprintf(codef, "\tconst %s dst = 0;\n", cgSize(size));
	cgFetchDstEa(eano, eareg_cpu_data_index, size);

        if (stricmp(i.instruction_name, "CLR") == 0 && (eano >= 2 && eano < 11))
        {
          // CLR reads memory and throws away value, memory mapped IO sometimes need this read to happen.
          fprintf(codef, "\t%s(dstea);\n", cgMemoryFetch(size));
        }
      }
      else if (stricmp(i.instruction_name, "MOVEFROMSR") == 0)
      {
	fprintf(codef, "\tif (cpuGetModelMajor() > 0 && !cpuGetFlagSupervisor())\n");
	fprintf(codef, "\t{\n");
	fprintf(codef, "\t\tcpuThrowPrivilegeViolationException();\n");
	fprintf(codef, "\t\treturn;\n");
	fprintf(codef, "\t}\n");

	cgFetchDstEa(eano, eareg_cpu_data_index, size);
      }
      else if (regtype == 'S' || regtype == 'C' || regtype == 'H')
      {
	cgFetchDstEa(eano, eareg_cpu_data_index, size);
      }
      else if (stricmp(i.instruction_name, "MOVEM") != 0 && regtype != 'R')
      {
	if (stricmp(i.instruction_name, "TST") == 0 && size == 2 && eano == 1)
	{
	  // Special case, TST.W Ax
	  fprintf(codef, "\uint16_t dst = (uint16_t)cpuGetAReg(opc_data[%d]);\n", eareg_cpu_data_index);
	}
	else
	{
	  cgFetchDst(eano, eareg_cpu_data_index, size);
	}
      }

      // Generate operation
      if (stricmp(i.instruction_name, "MOVEM") == 0)
      {
	if (regtype == 'S')
	  fprintf(codef, "\t%s(regs, dstea, opc_data[%d]);\n", i.function, eatime_cpu_data_index);
	else
	  fprintf(codef, "\t%s(regs, opc_data[%d]);\n", i.function, eareg_cpu_data_index);
      }
      else if ((stricmp(i.instruction_name, "MULL") == 0)
	       || (stricmp(i.instruction_name, "MOVES") == 0)
	       || (stricmp(i.instruction_name, "CAS") == 0)
	       || (stricmp(i.instruction_name, "CHKCMP2") == 0)
	       || (stricmp(i.instruction_name, "CALLM") == 0)
	       || (stricmp(i.instruction_name, "PFLUSH030") == 0)
	       || (strnicmp(i.instruction_name, "BF", 2) == 0))
      {
	if (regtype == 'S')
	{
	  fprintf(codef, "\t%s(dstea, ext);\n", i.function);
	}
	else if (stricmp(i.instruction_name, "MULL") == 0 || stricmp(i.instruction_name, "MOVES") == 0)
	{
	  fprintf(codef, "\t%s(src, ext);\n", i.function);
	}
	else if ((strnicmp(i.instruction_name, "BF", 2) == 0) && regtype == 'R')
	{
	  fprintf(codef, "\t%s(opc_data[%d], ext);\n", i.function, eareg_cpu_data_index);
	}
        else
        {
	  fprintf(codef, "\t%s(opc_data[%d], ext);\n", i.function, cc_cpu_data_index);
        }
      }
      else if (regtype == 'G')
      {
	fprintf(codef, "\t%s(src);\n", i.function);
      }
      else if (regtype == '0')
      {
	fprintf(codef, "\t%s();\n", i.function);
	// Generate store
	cgStoreDst(eano, eareg_cpu_data_index, size, "dst");
      }
      else if (regtype == 'T')
      {
	fprintf(codef, "\t%s(dst);\n", i.function);
      }
      else if (regtype == 'S')
      {
	fprintf(codef, "\t%s(dstea);\n", i.function);
      }
      else if (regtype == 'C')
      {
	fprintf(codef, "\t%s dst = %s(opc_data[%d]);\n", cgSize(size), i.function, cc_cpu_data_index);
	// Generate store
	cgStoreDst(eano, eareg_cpu_data_index, size, "dst");
      }
      else if (regtype == 'R')
      {
	fprintf(codef, "\t%s(opc_data[%d]);\n", i.function, cc_cpu_data_index);
      }
      else if (regtype == 'H')
      {
	fprintf(codef, "\t%s dst = %s();\n", cgSize(size), i.function);
	// Generate store
	cgStoreDst(eano, eareg_cpu_data_index, size, "dst");
      }
      else
      {
	fprintf(codef, "\tdst = %s(dst);\n", i.function);
	// Generate store
	cgStoreDst(eano, eareg_cpu_data_index, size, "dst");
      }

      // Set cycle times
      if (stricmp(i.instruction_name, "NBCD") == 0)
      {
        cgMakeInstructionTimeAbs(((eano == 0) ? 6 : 8) + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
      }
      else if ((stricmp(i.instruction_name, "CLR") == 0)
	       || (stricmp(i.instruction_name, "NOT") == 0)
	       || (stricmp(i.instruction_name, "NEG") == 0)
	       || (stricmp(i.instruction_name, "NEGX") == 0))
      {
	unsigned int base_time;
	if (size <= 2) base_time = (eano == 0) ? 4 : 8;
	else base_time = (eano == 0) ? 6 : 12;
        cgMakeInstructionTimeAbs(base_time + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
      }
      else if (stricmp(i.instruction_name, "TST") == 0)
      {
        cgMakeInstructionTimeAbs(4 + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
      }
      else if (stricmp(i.instruction_name, "JSR") == 0)
      {
        cgMakeInstructionTimeAbs(cg_jsr_time[eano]);
      }
      else if (stricmp(i.instruction_name, "JMP") == 0)
      {
        cgMakeInstructionTimeAbs(cg_jmp_time[eano]);
      }
      else if (stricmp(i.instruction_name, "PEA") == 0)
      {
        cgMakeInstructionTimeAbs(cg_pea_time[eano]);
      }
      else if (stricmp(i.instruction_name, "TAS") == 0)
      {
        cgMakeInstructionTimeAbs(((eano == 0) ? 4 : 10) + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
      }
      else if ((stricmp(i.instruction_name, "MOVETOSR") == 0)
	       || (stricmp(i.instruction_name, "MOVETOCCR") == 0))
      {
        cgMakeInstructionTimeAbs(12 + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
      }
      else if ((stricmp(i.instruction_name, "MOVEFROMSR") == 0)
	       || (stricmp(i.instruction_name, "MOVEFROMCCR") == 0))
      {
	cgMakeInstructionTimeAbs(((eano == 0) ? 6 : 8) + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
      }
      else if (stricmp(i.instruction_name, "SCC") == 0)
      {
	if (eano == 0)
	{
	  fprintf(codef, "\tif (dst == 0)\n\t{\n");
	  cgMakeInstructionTimeAbs(4 + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
	  fprintf(codef, "\t}\n\telse\n\t{\n");
	  cgMakeInstructionTimeAbs(6 + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
	  fprintf(codef, "\t}\n");
	}
	else
	{
	  cgMakeInstructionTimeAbs(8 + cg_ea_time[cgGetSizeCycleIndex(size)][eano]);
	}
      }

      cgMakeFunctionFooter(fname, templ_name);

      // Cycle the registers to set function ptr, data/reg and eareg for each opcode.
      for (cc = 0; (regtype == 'C' && cc < 16) || cc == 0; ++cc)
      {
	for (eareg = 0; (eano < 7 && eareg < 8) || (eano >= 7 && eareg == 0); ++eareg)
	{
	  unsigned int opcode2 = opcode | eareg | ((regtype == 'C') ? (cc << 8) : 0);
	  cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	  cgSetData(opcode2, eareg_cpu_data_index, (eano <7) ? eareg : 0);

	  if (regtype == 'C') cgSetData(opcode2, cc_cpu_data_index, cc);

	  if (stricmp(i.instruction_name, "MOVEM") == 0 && regtype == 'S')
	  {
	    // movem adds this to the time taken to move registers.
	    if (stricmp(i.description, "RxToEa") == 0)
	      cgSetData(opcode2, eatime_cpu_data_index, cg_movemtomem_time[cgGetEaNo(eano, eareg)]);
	    else
	      cgSetData(opcode2, eatime_cpu_data_index, cg_movemtoreg_time[cgGetEaNo(eano, eareg)]);
	  }
	  cgSetModelMask(opcode2, i.cpu_model_mask);
	  cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
	  totalcount++;
	}
      }
    }
  }
  return icount;
}

// MOVEQ
unsigned int cgMoveQ(cpu_data *cpudata, cpu_instruction_info i)
{
  char *templ_name = "moveq";
  char *endchar;
  unsigned int base_opcode = strtoul(i.opcode, &endchar, 0);
  unsigned int reg_cpu_data_index = 0;
  unsigned int imm_cpu_data_index = 1;
  unsigned int flags_cpu_data_index = 2;
  int reg;
  unsigned int icount = 0, imm;

  char fname[32];
  cgMakeFunctionName(fname, i.instruction_name, base_opcode);
  cgDeclareFunction(fname);
  cgMakeFunctionHeader(fname, templ_name);

  fprintf(codef, "\tcpuSetDReg(opc_data[%d], opc_data[%d]);\n", reg_cpu_data_index, imm_cpu_data_index);
  fprintf(codef, "\tcpuSetFlagsAbs((uint16_t)opc_data[%d]);\n", flags_cpu_data_index);
  cgMakeInstructionTimeAbs(4);
  cgMakeFunctionFooter(fname, templ_name);

  icount++;

  for (imm = 0; imm < 256; ++imm)
  {
    for (reg = 0; reg < 8; ++reg)
    {
      unsigned int opcode = base_opcode | imm | (reg << 9);
      cgCopyFunctionName(fname, cpudata[opcode].name, templ_name);
      cgSetData(opcode, reg_cpu_data_index, reg);
      cgSetData(opcode, imm_cpu_data_index, (unsigned int)(int)(char)imm);
      cgSetData(opcode, flags_cpu_data_index, ((imm & 0x80) ? 8:0) | ((imm == 0) ? 4:0));
      cgSetModelMask(opcode, i.cpu_model_mask);
      cgSetDisassemblyFunction(opcode, i.disasm_func_no);
    }
  }
  return icount;
}

// ADDX, SUBX, ABCD, SBCD
unsigned int cgAddx(cpu_data *cpudata, cpu_instruction_info i)
{
  char *templ_name = "addx";
  char *endchar;
  unsigned int base_opcode = strtoul(i.opcode, &endchar, 0);
  unsigned int regx_cpu_data_index = 0;
  unsigned int regy_cpu_data_index = 1;
  char regtype = i.reg[0];
  unsigned int eano = (regtype == 'D') ? 0 : 4;
  unsigned int regx;
  unsigned int regy;
  unsigned int icount = 0;
  unsigned int size = atoi(i.size);

  char fname[32];
  cgMakeFunctionName(fname, i.instruction_name, base_opcode);
  cgDeclareFunction(fname);
  cgMakeFunctionHeader(fname, templ_name);

  cgFetchSrc(eano, regx_cpu_data_index, size, 'D', 0);
  cgFetchDst(eano, regy_cpu_data_index, size);
  fprintf(codef, "\tdst = %s(dst, src);\n", i.function);
  cgStoreDst(eano, regy_cpu_data_index, size, "dst");
  if ((stricmp(i.instruction_name, "ABCD") == 0) || (stricmp(i.instruction_name, "SBCD") == 0))
  {
    cgMakeInstructionTimeAbs((regtype == 'D') ? 6 : 18);
  }
  else
  {
    cgMakeInstructionTimeAbs((regtype == 'D') ? ((size <= 2) ? 4:8) : ((size <= 2) ? 18:30));
  }
  cgMakeFunctionFooter(fname, templ_name);
  icount++;

  for (regx = 0; regx < 8; ++regx)
  {
    for (regy = 0; regy < 8; ++regy)
    {
      unsigned int opcode = base_opcode | regx | (regy << 9);
      cgCopyFunctionName(fname, cpudata[opcode].name, templ_name);
      cgSetData(opcode, regx_cpu_data_index, regx);
      cgSetData(opcode, regy_cpu_data_index, regy);
      cgSetModelMask(opcode, i.cpu_model_mask);
      cgSetDisassemblyFunction(opcode, i.disasm_func_no);
    }
  }
  return icount;
}

// MOVEP, PACK (020), UNPK (020)
unsigned int cgMovep(cpu_data *cpudata, cpu_instruction_info i)
{
  char *templ_name = "movep";
  char *endchar;
  unsigned int base_opcode = strtoul(i.opcode, &endchar, 0);
  unsigned int regx_cpu_data_index = 0;
  unsigned int regy_cpu_data_index = 1;
  char regtype = i.reg[0];
  unsigned int regx;
  unsigned int regy;
  unsigned int icount = 0;

  char fname[32];
  cgMakeFunctionName(fname, i.instruction_name, base_opcode);
  cgDeclareFunction(fname);
  cgMakeFunctionHeader(fname, templ_name);

  fprintf(codef, "\t%s(opc_data[%d], opc_data[%d]);\n", i.function, regy_cpu_data_index, regx_cpu_data_index);
  cgMakeFunctionFooter(fname, templ_name);
  icount++;

  for (regx = 0; regx < 8; ++regx)
  {
    for (regy = 0; regy < 8; ++regy)
    {
      unsigned int opcode = base_opcode | regy | (regx << 9);
      cgCopyFunctionName(fname, cpudata[opcode].name, templ_name);
      cgSetData(opcode, regx_cpu_data_index, regx);
      cgSetData(opcode, regy_cpu_data_index, regy);
      cgSetModelMask(opcode, i.cpu_model_mask);
      cgSetDisassemblyFunction(opcode, i.disasm_func_no);
    }
  }
  return icount;
}

// Bcc (020 features implemented run-time)
// BKPT (Actually 010+)
// BRA (020 features implemented run-time)
// BSR (020 features implemented run-time)
// DBcc
// RTR
// RTS
// STOP
// TRAPV
// RTM
unsigned int cgBCC(cpu_data *cpudata, cpu_instruction_info i)
{
  char *templ_name = "bcc";
  char *endchar;
  unsigned int base_opcode = strtoul(i.opcode, &endchar, 0);
  unsigned int cc_cpu_data_index = 0;
  unsigned int vector_cpu_data_index = 0;
  unsigned int offset_cpu_data_index = 1;
  unsigned int reg_cpu_data_index = 1;
  unsigned int reg1_cpu_data_index = 0;
  unsigned int reg2_cpu_data_index = 1;
  unsigned int icount = 0;
  unsigned int size = atoi(i.size);
  unsigned int offset;
  char fname[32];

  cgMakeFunctionName(fname, i.instruction_name, base_opcode);
  cgDeclareFunction(fname);
  cgMakeFunctionHeader(fname, templ_name);
  
  if (stricmp(i.instruction_name, "STOP") == 0)
  {
    fprintf(codef, "\t%s(cpuGetNextWord());\n", i.function);
  }
  else if ((stricmp(i.instruction_name, "BKPT") == 0)
           || (stricmp(i.instruction_name, "SWAP") == 0)
           || (stricmp(i.instruction_name, "LINK") == 0)
	   || (stricmp(i.instruction_name, "UNLK") == 0)
	   || (stricmp(i.instruction_name, "TRAP") == 0)
	   || (stricmp(i.instruction_name, "EXT") == 0))
  {
    fprintf(codef, "\t%s(opc_data[%d]);\n", i.function, vector_cpu_data_index);
  }
  else if ((stricmp(i.instruction_name, "EXG") == 0)
	   || (stricmp(i.instruction_name, "CMPM") == 0))
  {
    fprintf(codef, "\t%s(opc_data[%d], opc_data[%d]);\n", i.function, reg1_cpu_data_index, reg2_cpu_data_index);
  }
  else if ((stricmp(i.instruction_name, "RTM") == 0)
	  || (stricmp(i.instruction_name, "PFLUSH040") == 0)
	  || (stricmp(i.instruction_name, "PTEST040") == 0))
  {
    fprintf(codef, "\t%s(opc_data[%d], opc_data[%d]);\n", i.function, reg1_cpu_data_index, reg2_cpu_data_index);
  }
  else if (size == 1 && (stricmp(i.instruction_name, "BCCB") == 0))
  { // BCC.b
    fprintf(codef, "\t%s(cpuCalculateConditionCode%s(), opc_data[%d]);\n", i.function, i.reg, offset_cpu_data_index);
  }
  else if (size == 1 && i.reg[0] != 'C')
  { // BRA, BSR
    fprintf(codef, "\t%s(opc_data[%d]);\n", i.function, offset_cpu_data_index);
  }
  else if (size == 2 && (stricmp(i.instruction_name, "DBCC") == 0))
  { // DBCC
    fprintf(codef, "\t%s(cpuCalculateConditionCode%s(), opc_data[%d]);\n", i.function, i.reg, reg_cpu_data_index);
  }
  else if ((size >= 2 && (strnicmp(i.instruction_name, "BCC", 3) == 0)) || (strnicmp(i.instruction_name, "TRAPCC", 6) == 0))
  { // BCC.wl (offset is in the extension word), TRAPCC
    //fprintf(codef, "\t%s(opc_data[%d]);\n", i.function, cc_cpu_data_index);
    fprintf(codef, "\t%s(cpuCalculateConditionCode%s());\n", i.function, i.reg);
  }
  else if (stricmp(i.instruction_name, "MOVEUSP") == 0)
  {
    fprintf(codef, "\t%s(opc_data[%d]);\n", i.function, reg1_cpu_data_index);
  }
  else
  { // BRAWL, BSRWL
    fprintf(codef, "\t%s();\n", i.function);
  }
  cgMakeFunctionFooter(fname, templ_name);
  icount++;

  if ((stricmp(i.instruction_name, "BKPT") == 0)
      || (stricmp(i.instruction_name, "SWAP") == 0)
      || (stricmp(i.instruction_name, "LINK") == 0)
      || (stricmp(i.instruction_name, "UNLK") == 0)
      || (stricmp(i.instruction_name, "TRAP") == 0)
      || (stricmp(i.instruction_name, "EXT") == 0))
  {
    unsigned int j;
    unsigned int maxvalue;
    if (stricmp(i.instruction_name, "TRAP") == 0) maxvalue = 16;
    else maxvalue = 8;
    for (j = 0; j < maxvalue; ++j)
    {
      unsigned int opcode2 = base_opcode | j;
      cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
      cgSetData(opcode2, vector_cpu_data_index, j);
      cgSetModelMask(opcode2, i.cpu_model_mask);
      cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
    }
  }
  else if ((stricmp(i.instruction_name, "EXG") == 0)
           || (stricmp(i.instruction_name, "CMPM") == 0))
  {
    unsigned int j, k;
    for (j = 0; j < 8; ++j)
      for (k = 0; k < 8; ++k)
      {
	unsigned int opcode2 = base_opcode | j << 9 | k;
	cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	cgSetData(opcode2, reg1_cpu_data_index, j);
	cgSetData(opcode2, reg2_cpu_data_index, k);
	cgSetModelMask(opcode2, i.cpu_model_mask);
	cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
      }
  }
  else if (stricmp(i.instruction_name, "RTM") == 0)
  {
    unsigned int j, k;
    for (j = 0; j < 2; ++j)
      for (k = 0; k < 8; ++k)
      {
	unsigned int opcode2 = base_opcode | j << 3 | k;
	cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	cgSetData(opcode2, reg1_cpu_data_index, j);
	cgSetData(opcode2, reg2_cpu_data_index, k);
	cgSetModelMask(opcode2, i.cpu_model_mask);
	cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
      }
  }
  else if (stricmp(i.instruction_name, "PFLUSH040") == 0)
  {
    unsigned int j, k;
    for (j = 0; j < 4; ++j)
      for (k = 0; k < 8; ++k)
      {
	unsigned int opcode2 = base_opcode | j << 3 | k;
	cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	cgSetData(opcode2, reg1_cpu_data_index, j);
	cgSetData(opcode2, reg2_cpu_data_index, k);
	cgSetModelMask(opcode2, i.cpu_model_mask);
	cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
      }
  }
  else if (stricmp(i.instruction_name, "PTEST040") == 0)
  {
    unsigned int j, k;
    for (j = 0; j < 2; ++j)
      for (k = 0; k < 8; ++k)
      {
	unsigned int opcode2 = base_opcode | j << 5 | k;
	cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	cgSetData(opcode2, reg1_cpu_data_index, j);
	cgSetData(opcode2, reg2_cpu_data_index, k);
	cgSetModelMask(opcode2, i.cpu_model_mask);
	cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
      }
  }
  else if (size == 1 && ((stricmp(i.instruction_name, "BCCB") == 0) || 
		         (strnicmp(i.instruction_name, "BRA", 3) == 0) || 
			 (strnicmp(i.instruction_name, "BSR", 3) == 0)))
  {
    // BRAB, BSRB, BCCB (offset 1 - 254)
    for (offset = 1; offset < 255; ++offset)
    {
      unsigned int opcode = base_opcode | offset;
      cgCopyFunctionName(fname, cpudata[opcode].name, templ_name);
      cgSetData(opcode, offset_cpu_data_index, (unsigned int)(int)(char)offset);
      cgSetModelMask(opcode, i.cpu_model_mask);
      cgSetDisassemblyFunction(opcode, i.disasm_func_no);
    }
  }
  else if ((strnicmp(i.instruction_name, "TRAPCC", 6) == 0)
           || (size >= 2 && ((strnicmp(i.instruction_name, "BCC", 3) == 0) || (strnicmp(i.instruction_name, "BRA", 3) == 0) || (strnicmp(i.instruction_name, "BSR", 3) == 0))))
  { // BCCWL, BRAWL, BSRWL, TRAPCC
    unsigned int opcode = base_opcode;
    cgCopyFunctionName(fname, cpudata[opcode].name, templ_name);
    cgSetModelMask(opcode, i.cpu_model_mask);
    cgSetDisassemblyFunction(opcode, i.disasm_func_no);
  }
  else if (stricmp(i.instruction_name, "DBCC") == 0)
  {
    // DBCC - reg and cc
    unsigned int reg;
    for (reg = 0; reg < 8; ++reg)
    {
      unsigned int opcode = base_opcode | reg;
      cgCopyFunctionName(fname, cpudata[opcode].name, templ_name);
      cgSetData(opcode, reg_cpu_data_index, reg);
      cgSetModelMask(opcode, i.cpu_model_mask);
      cgSetDisassemblyFunction(opcode, i.disasm_func_no);
    }
  }
  else if (stricmp(i.instruction_name, "MOVEUSP") == 0)
  {
    unsigned int reg;
    for (reg = 0; reg < 8; ++reg)
    {
      unsigned int opcode = base_opcode | reg;
      cgCopyFunctionName(fname, cpudata[opcode].name, templ_name);
      cgSetData(opcode, reg1_cpu_data_index, reg);
      cgSetModelMask(opcode, i.cpu_model_mask);
      cgSetDisassemblyFunction(opcode, i.disasm_func_no);
    }
  }
  else
  {
    // CAS2
    cgCopyFunctionName(fname, cpudata[base_opcode].name, templ_name);
    cgSetModelMask(base_opcode, i.cpu_model_mask);
    cgSetDisassemblyFunction(base_opcode, i.disasm_func_no);
  }
  return icount;
}

// LSL,LSR,ASL,ASR,ROL,ROR,ROXL,ROXR
//
// Timing information:
//
// Registers: 6 (word) / 8 (long) + 2 x shift count
// Memory:    8 + EA time + 2 x 1 (shift count)
//
unsigned int cgShift(cpu_data *cpudata, cpu_instruction_info i)
{
  char *templ_name = "shift";
  char *endchar;
  unsigned int base_opcode = strtoul(i.opcode, &endchar, 0);
  unsigned int eareg_cpu_data_index = 0;
  unsigned int shift_cpu_data_index = 1;
  unsigned int time_cpu_data_index = 2;
  unsigned int size = atoi(i.size);
  int eano, eareg, reg;
  unsigned int icount = 0;
  char regtype = i.reg[0];
  char fname[32];

  // For each EA in i.eamask
  for (eano = 0; eano < 12; ++eano)
  {
    if (i.eamask[eano] == '1')
    {
      unsigned int opcode = base_opcode | cgEAReg(eano, 0);

      cgMakeFunctionName(fname, i.instruction_name, opcode);
      cgDeclareFunction(fname);
      cgMakeFunctionHeader(fname, templ_name);
      cgFetchDst(eano, eareg_cpu_data_index, size);
      if (regtype == 'I') /* Immediate shift */
      {
	fprintf(codef, "\tdst = %s(dst, opc_data[%d], opc_data[%d]);\n", i.function, shift_cpu_data_index, time_cpu_data_index);
      }
      else if (regtype == 'D') /* Shift count in a register */
      {
	fprintf(codef, "\tdst = %s(dst, cpuGetDReg(opc_data[%d]), opc_data[%d]);\n", i.function, shift_cpu_data_index, time_cpu_data_index);
      }
      else /* Shift one */
      {
	fprintf(codef, "\tdst = %s(dst, 1, opc_data[%d]);\n", i.function, time_cpu_data_index);
      }
      cgStoreDst(eano, eareg_cpu_data_index, size, "dst");
      cgMakeFunctionFooter(fname, templ_name);

      for (eareg = 0; (eano < 7 && eareg < 8) || (eano >= 7 && eareg == 0); eareg++)
      {
	if (regtype != '1')
	{
	  for (reg = 0; reg <8; reg++)
	  {
	    unsigned int opcode2 = base_opcode | (reg << 9) | eareg;
	    icount++;
	    cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	    cgSetData(opcode2, eareg_cpu_data_index, (eano < 7) ? eareg : 0);
	    if (regtype == 'I') cgSetData(opcode2, shift_cpu_data_index, (reg == 0) ? 8 : reg);
	    else cgSetData(opcode2, shift_cpu_data_index, reg);
	    cgSetData(opcode2, time_cpu_data_index, (size == 4) ? 8 : 6);
	    cgSetModelMask(opcode2, i.cpu_model_mask);
	    cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
	  }
	}
	else
	{
	    unsigned int opcode2 = base_opcode | cgEAReg(eano, eareg);
	    icount++;
	    cgCopyFunctionName(fname, cpudata[opcode2].name, templ_name);
	    cgSetData(opcode2, eareg_cpu_data_index, (eano < 7) ? eareg : 0);
	    cgSetData(opcode2, time_cpu_data_index, 6 + cg_ea_time[1][cgGetEaNo(eano, eareg)]);
	    cgSetModelMask(opcode2, i.cpu_model_mask);
	    cgSetDisassemblyFunction(opcode2, i.disasm_func_no);
	}
      }
    }
  }
  return icount;
}

unsigned int cgInstruction(cpu_instruction_info i)
{
  if (stricmp(i.format, "ADD") == 0) return cgAdd(cpu_opcode_data, i);
  else if (stricmp(i.format, "CLR") == 0) return cgClr(cpu_opcode_data, i);
  else if (stricmp(i.format, "MOVEQ") == 0) return cgMoveQ(cpu_opcode_data, i);
  else if (stricmp(i.format, "MOVE") == 0) return cgMove(cpu_opcode_data, i);
  else if (stricmp(i.format, "ADDX") == 0) return cgAddx(cpu_opcode_data, i);
  else if (stricmp(i.format, "BCC") == 0) return cgBCC(cpu_opcode_data, i);
  else if (stricmp(i.format, "SHIFT") == 0) return cgShift(cpu_opcode_data, i);
  else if (stricmp(i.format, "MOVEP") == 0) return cgMovep(cpu_opcode_data, i);
  return 0;
}

unsigned int cgInstructions()
{
  unsigned int i;
  unsigned int icount = 0;

  for (i = 0; i < cpuinfo_count; i++)
  {
    unsigned int count = cgInstruction(cpu_opcode_info[i]);
    printf("Generated %d opcodes for %s.\n", count, cpu_opcode_info[i].instruction_name);
    icount += count;
  }
  return icount;
}

void cgClearCpuData()
{
  unsigned int i;
  memset(cpu_opcode_data, 0, sizeof(cpu_data)*65536);
  for (i = 0; i < 65536; ++i) strcpy(cpu_opcode_data[i].name, "cpuIllegalInstruction");
}

void cgData()
{
  unsigned int i;

  fprintf(dataf, "typedef void (*cpuInstructionFunction)(uint32_t*);\n");
  fprintf(dataf, "typedef struct cpu_data_struct\n{\n");
  fprintf(dataf, "\tcpuInstructionFunction instruction_func;\n");
  fprintf(dataf, "\tuint32_t data[3];\n} cpuOpcodeData;\n\n");

  fprintf(dataf, "cpuOpcodeData cpu_opcode_data[65536] = {\n");
  for (i = 0; i < 65536; ++i)
  {
    fprintf(dataf, "{%s,{%uU,%uU,%uU}}%s\n", cpu_opcode_data[i].name, cpu_opcode_data[i].data[0], cpu_opcode_data[i].data[1], cpu_opcode_data[i].data[2], ((i == 65535) ? "" : ","));
  }
  fprintf(dataf, "};\n");
  fprintf(dataf, "uint8_t cpu_opcode_model_mask[65536] = {\n");
  for (i = 0; i < 65536; i = i + 16)
  {
    fprintf(dataf, 
	    "0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X%s\n",
	    cpu_opcode_model_mask[i],
	    cpu_opcode_model_mask[i+1],
	    cpu_opcode_model_mask[i+2],
	    cpu_opcode_model_mask[i+3],
	    cpu_opcode_model_mask[i+4],
	    cpu_opcode_model_mask[i+5],
	    cpu_opcode_model_mask[i+6],
	    cpu_opcode_model_mask[i+7],
	    cpu_opcode_model_mask[i+8],
	    cpu_opcode_model_mask[i+9],
	    cpu_opcode_model_mask[i+10],
	    cpu_opcode_model_mask[i+11],
	    cpu_opcode_model_mask[i+12],
	    cpu_opcode_model_mask[i+13],
	    cpu_opcode_model_mask[i+14],
	    cpu_opcode_model_mask[i+15],
	    (i == 65520) ? "" : ",");
  }
  fprintf(dataf, "};\n\n");
} 

void cgCloseFiles()
{
  if (codef != NULL)
  {
    fprintf(codef, "#endif\n");
    fclose(codef);
  }
  if (dataf != NULL)
  {
    fprintf(dataf, "#endif\n");
    fclose(dataf);
  }
  if (declf != NULL)
  {
    fprintf(declf, "#endif\n");
    fclose(declf);
  }
  if (profilef != NULL)
  {
    fprintf(profilef, "#endif\n");
    fclose(profilef);
  }
  if (disfuncf != NULL)
  {
    fprintf(disfuncf, "#endif\n");
    fclose(disfuncf);
  }
}

int cgOpenFiles()
{
  int res;
  codef = fopen(cpucode_path, "w");
  dataf = fopen(cpudata_path, "w");
  declf = fopen(cpudecl_path, "w");
  disfuncf = fopen(cpudisfunc_path, "w");
  profilef = fopen(cpuprofile_path, "w");
  res = (codef != NULL && dataf != NULL && declf != NULL && profilef != NULL && disfuncf != NULL);
  if (!res) cgCloseFiles();

  fprintf(dataf, "#ifndef CPUMODULE_DATA_H\n");
  fprintf(dataf, "#define CPUMODULE_DATA_H\n\n");
  fprintf(codef, "#ifndef CPUMODULE_CODE_H\n");
  fprintf(codef, "#define CPUMODULE_CODE_H\n\n");
  fprintf(declf, "#ifndef CPUMODULE_DECL_H\n");
  fprintf(declf, "#define CPUMODULE_DECL_H\n\n");
  fprintf(profilef, "#ifndef CPUMODULE_PROFILE_H\n");
  fprintf(profilef, "#define CPUMODULE_PROFILE_H\n\n");
  fprintf(disfuncf, "#ifndef CPUMODULE_DISASSEMBLERFUNC_H\n");
  fprintf(disfuncf, "#define CPUMODULE_DISASSEMBLERFUNC_H\n\n");

  return res;
}

int cgReadControlFile(char *filename)
{
  FILE *F = fopen(filename, "r");
  unsigned int res = 0;
  if (F == NULL) return 0;
  do
  {
    res = fscanf(F, 
		 "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", 
		  cpu_opcode_info[cpuinfo_count].instruction_name,
		  cpu_opcode_info[cpuinfo_count].cpu_model_mask,
		  cpu_opcode_info[cpuinfo_count].description,
		  cpu_opcode_info[cpuinfo_count].function,
		  cpu_opcode_info[cpuinfo_count].format,
		  cpu_opcode_info[cpuinfo_count].size,
		  cpu_opcode_info[cpuinfo_count].opcode,
		  cpu_opcode_info[cpuinfo_count].eamask,
		  cpu_opcode_info[cpuinfo_count].eaisdest,
		  cpu_opcode_info[cpuinfo_count].reg,
		  cpu_opcode_info[cpuinfo_count].eamask2,
		  cpu_opcode_info[cpuinfo_count].disasm_func_no);
    if (res != EOF) cpuinfo_count++;
  } while (res != EOF);
  fclose(F);
  return 1;
}

int cgMain(char *definition_file, char *include_path)
{
  sprintf(cpucode_path, "%s\\CpuModule_Code.h", include_path);
  sprintf(cpudata_path, "%s\\CpuModule_Data.h", include_path);
  sprintf(cpudecl_path, "%s\\CpuModule_Decl.h", include_path);
  sprintf(cpuprofile_path, "%s\\CpuModule_Profile.h", include_path);
  sprintf(cpudisfunc_path, "%s\\CpuModule_DisassemblerFunc.h", include_path);

  if (!cgReadControlFile(definition_file)) return 0;
  if (!cgOpenFiles()) return 0;

  cgClearCpuData();

  cgInstructions();
  cgData();
  cgDisFunc();

  cgProfileLogHeader();
  cgProfileDeclarations();
  cgProfileLogFooter();

  cgCloseFiles();
  printf("Generated %d opcodes.\n", totalcount);
  return 1;
}


void main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Usage:\nWinFellow.Generate68k <definition file> <code destination path>\n\n");
    return;
  }
  if (!cgMain(argv[1], argv[2]))
  {
    printf("WinFellow.Generate68k: Invalid path\n");
  }
}

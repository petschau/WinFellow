/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* CPU disassembly                                                            */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include "portable.h"
#include "renaming.h"
#include "defs.h"
#include "fmem.h"
#include "cpu.h"




/*===========================================================================
  Disassembly of the address-modes
  Parameters:
   ULO reg  - The register used in the address-mode
   ULO pcp  - The address of the next byte not used.
   STR *st - The string to write the dissassembly to.
   ULO *pos - The position in the string where the hex-words used are written
  Returnvalue:
   PC after possible extension words
  ===========================================================================*/

void disWordPrint(STR *st, ULO *pos, ULO data) {
  sprintf(&st[*pos], "%.4X", data);
  *pos += 5;
  st[(*pos) - 1] = ' '; 
}  

void disLongPrint(STR *st, ULO *pos, ULO data) {
  sprintf(&st[*pos], "%.8X", data);
  *pos += 9;
  st[(*pos) - 1] = ' ';
}  

ULO dis00(ULO reg, ULO pcp, STR *st, ULO *pos) {
  return pcp;
}

ULO dis01(ULO reg, ULO pcp, STR *st, ULO *pos) {
  sprintf(st, "%sA%1d", st, reg);
  return pcp;
}

ULO dis02(ULO reg, ULO pcp, STR *st, ULO *pos) {
  sprintf(st, "%s(A%1d)", st, reg);
  return pcp;
}

ULO dis03(ULO reg, ULO pcp, STR *st, ULO *pos) {
  sprintf(st, "%s(A%1d)+", st, reg);
  return pcp;
}

ULO dis04(ULO reg, ULO pcp, STR *st, ULO *pos) {
  sprintf(st, "%s-(A%1d)", st, reg);
  return pcp;
}

ULO dis05(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO j = fetw(pcp);

  sprintf(st, "%s$%.4X(A%1d)", st, j, reg);
  disWordPrint(st, pos, j);
  return pcp + 2;
}

ULO dis06Brief(ULO reg, ULO pcp, STR *st, ULO *pos, BOOLE pcadr, ULO ext) {
  ULO scale[4] = {1, 2, 4, 8};
  char indexregtype = (char) ((ext & 0x8000) ? 'A' : 'D');
  char indexsize = (char) ((ext & 0x0800) ? 'L' : 'W');
  ULO indexreg = (ext & 0x7000)>>12;
  ULO offset = ext & 0xff;
  ULO scalefactor = (ext & 0x0600)>>9;
  
  if (cpuGetType() < 2) {
    if (!pcadr)
      sprintf(st, "%s$%.2X(A%1d,%c%1d.%c)", st, offset, reg,
	      indexregtype, indexreg, indexsize);
    else
      sprintf(st, "%s$%.2X(PC,%c%1d.%c)", st, offset,
	      indexregtype, indexreg, indexsize);
  }
  else {
    if (!pcadr)
      sprintf(st, "%s$%.2X(A%1d,%c%1d.%c*%d)", st, offset, reg,
	      indexregtype, indexreg, indexsize, scale[scalefactor]);
    else
      sprintf(st, "%s$%.2X(PC,%c%1d.%c*%d)", st, offset,
	      indexregtype, indexreg, indexsize, scale[scalefactor]);
  }
  disWordPrint(st, pos, ext);
  return pcp;
}

ULO dis06od(ULO pcp, STR *st, ULO *pos, BOOLE wordsize, STR *dispstr) {
  ULO od;

  if (wordsize) {
    od = fetw(pcp);
    pcp += 2;
    disWordPrint(st, pos, od);
    sprintf(dispstr, "$%.4X", od);
  }
  else {
    od = fetl(pcp);
    pcp += 4;
    disLongPrint(st, pos, od);
    sprintf(dispstr, "$%.8X", od);
  }
  return pcp;
}

ULO dis06Extended(ULO reg, ULO pcp, STR *st, ULO *pos, BOOLE pcadr, ULO ext) {
  ULO scale[4] = {1, 2, 4, 8};
  char indexregtype = (char) ((ext & 0x8000) ? 'A' : 'D');
  char indexsize = (char)((ext & 0x0800) ? 'L' : 'W');
  ULO indexreg = (ext & 0x7000)>>12;
  ULO scalefactor = (ext & 0x0600)>>9;
  ULO iis = ext & 0x0007;
  ULO bdsize = (ext & 0x0030)>>4;
  ULO bd;
  BOOLE is = !!(ext & 0x0040);
  BOOLE bs = !!(ext & 0x0080);
  STR baseregstr[10];
  STR indexstr[10];
  STR basedispstr[10];
  STR outerdispstr[10];
  
  /* Print extension word */
  
  disWordPrint(st, pos, ext);

  /* Base register */

  if (bs) { /* Base register suppress */
    baseregstr[0] = '\0';
  }
  else {    /* Base register included */
    if (pcadr)
      strcpy(baseregstr, "PC");
    else
      sprintf(baseregstr, "A%d", reg);
  }

  /* Index register */

  if (is) { /* Index suppress */
    indexstr[0] = '\0';
  }
  else {    /* Index included */
    sprintf(indexstr, "%c%d.%c*%d", indexregtype, indexreg, indexsize,
	    scale[scalefactor]);
  }
  
  /* Base displacement */
  
  if (bdsize < 2) {
    basedispstr[0] = '\0';
  }
  else if (bdsize == 2) {
    bd = fetw(pcp);
    pcp += 2;
    disWordPrint(st, pos, bd);
    sprintf(basedispstr, "$%.4X", bd);
  }
  else if (bdsize == 3) {
    bd = fetl(pcp);
    pcp += 4;
    disLongPrint(st, pos, bd);
    sprintf(basedispstr, "$%.8X", bd);
  }
  
  switch (iis) { /* Evaluate and add index operand */
    case 0:           /* No memory indirect action */
      sprintf(st, "%s(%s,%s,%s)", st, basedispstr, baseregstr, indexstr);
      break;
    case 1:           /* Preindexed with null outer displacement */
      sprintf(st, "%s([%s,%s,%s])", st, basedispstr, baseregstr, indexstr);
      break;
    case 2:           /* Preindexed with word outer displacement */
      pcp = dis06od(pcp, st, pos, TRUE, outerdispstr);
      sprintf(st, "%s([%s,%s,%s],%s)", st, basedispstr, baseregstr, indexstr,
	      outerdispstr);
      break;
    case 3:           /* Preindexed with long outer displacement */
      pcp = dis06od(pcp, st, pos, FALSE, outerdispstr);
      sprintf(st, "%s([%s,%s,%s],%s)", st, basedispstr, baseregstr, indexstr,
	      outerdispstr);
      break;
    case 4:           /* Reserved ie. Illegal */
      sprintf(st, "%sRESERVED", st);
      break;
    case 5:           /* Postindexed with null outer displacement */
      if (is) sprintf(st, "%sRESERVED", st);
      else {
        sprintf(st, "%s([%s,%s],%s)", st, basedispstr, baseregstr, indexstr);
      }
      break;
    case 6:           /* Postindexed with word outer displacement */
      if (is) sprintf(st, "%sRESERVED", st);
      else {
        pcp = dis06od(pcp, st, pos, TRUE, outerdispstr);
        sprintf(st, "%s([%s,%s],%s,%s)", st, basedispstr, baseregstr, indexstr,
		outerdispstr);
      }
      break;
    case 7:           /* Postindexed with long outer displacement */
      if (is) sprintf(st, "RESERVED");
      else {
        pcp = dis06od(pcp, st, pos, TRUE, outerdispstr);
        sprintf(st, "%s([%s,%s],%s,%s)", st, basedispstr, baseregstr, indexstr,
		outerdispstr);
      }
      break;
  }
  return pcp;
}


ULO dis06(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO ext = fetw(pcp);

  if (cpuGetType() < 2 || !(ext & 0x0100))
    return dis06Brief(reg, pcp + 2, st, pos, FALSE, ext);
  else
    return dis06Extended(reg, pcp + 2, st, pos, FALSE, ext);
}

ULO dis70(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO j = fetw(pcp);

  sprintf(st, "%s$%.4X.W", st, j);
  disWordPrint(st, pos, j);
  return pcp + 2;
}

ULO dis71(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO j = fetl(pcp);

  sprintf(st, "%s$%.8X.L", st, j);
  disLongPrint(st, pos, j);
  return pcp + 4;
}

ULO dis72(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO j = fetw(pcp);

  sprintf(st, "%s$%.4X(PC)", st, j);
  disWordPrint(st, pos, j);
  return pcp + 2;
}

ULO dis73(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO ext = fetw(pcp);

  if (cpuGetType() < 2 || !(ext & 0x0100))
    return dis06Brief(reg, pcp, st, pos, TRUE, ext);
  else
    return dis06Extended(reg, pcp, st, pos, TRUE, ext);
}

ULO disb74(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO j = fetw(pcp);

  sprintf(st, "%s#$%.2X", st, j & 0x00ff);
  disWordPrint(st, pos, j);
  return pcp + 2;
}

ULO disw74(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO j = fetw(pcp);

  sprintf(st, "%s#$%.4X", st, j);
  disWordPrint(st, pos, j);
  return pcp + 2;
}

ULO disl74(ULO reg, ULO pcp, STR *st, ULO *pos) {
  ULO j = fetl(pcp);

  sprintf(st, "%s#$%.8X", st, j);
  disLongPrint(st, pos, j);
  return pcp + 4;
}

void disRegCat(BOOLE datareg, ULO reg, STR *st) {
  st[0] = (STR) ((datareg) ? 'D' : 'A');
  st[1] = (STR) (reg + 0x30);
  st[2] = '\0';
}

void disIndRegCat(ULO reg, STR *st, ULO type) {
  ULO i = 0;

  if (type == 2) st[i++] = '-';
  st[i++] = '(';
  st[i++] = 'A';
  st[i++] = (STR) (reg + 0x30);
  st[i++] = ')';
  if (type == 1) st[i++] = '+';
  st[i++] = '\0';
}



ULO disAdrMode(ULO reg, ULO pcp, STR *st, ULO size, ULO mode, ULO *pos) {
  ULO len = strlen(st);

  switch (mode) {
    case 0:
    case 1:  disRegCat(mode == 0, reg, st + len); break;
    case 2:
    case 3:
    case 4:  disIndRegCat(reg, st + len, mode - 2); break;
    case 5:  return dis05(reg, pcp, st, pos);
    case 6:  return dis06(reg, pcp, st, pos);
    case 7:  return dis70(reg, pcp, st, pos);
    case 8:  return dis71(reg, pcp, st, pos);
    case 9:  return dis72(reg, pcp, st, pos);
    case 10: return dis73(reg, pcp, st, pos);
    case 11: return (size == 8)  ? disb74(reg, pcp, st, pos):
                    ((size == 16) ? disw74(reg, pcp, st, pos):
                                   disl74(reg, pcp, st, pos));
    default: return pcp;
    }
  return pcp;
}

ULO disOpcode(ULO disasm_pc, STR *s) {
  UWO opcode = (UWO) fetw(disasm_pc);
  return cpu_dis_tab[opcode](disasm_pc, opcode, s);
}

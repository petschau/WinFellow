#ifndef CPUDIS_H
#define CPUDIS_H

extern ULO disAdrMode(ULO reg, ULO pcp, STR *st, ULO size, ULO mode, ULO *pos);
extern ULO disOpcode(ULO disasm_pc, STR *s);

#endif

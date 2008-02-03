
/****************************************************************************
 MEMORY RELATED FUNCTIONS
 ****************************************************************************/
/* Functions implemented in ma.asm */

/* TOP LEVEL INTERFACE */
extern ULO fetb(amigaadress ea);
extern ULO fetw(amigaadress ea);
extern ULO fetl(amigaadress ea);
extern void wrib(ULO data, amigaadress ea);
extern void wriw(ULO data, amigaadress ea);
extern void wril(ULO data, amigaadress ea);

extern void oddwrite(ULO data,amigaadress ea);
extern ULO oddread(amigaadress ea);

extern ULO wriorgadr;


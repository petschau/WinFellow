/* Return correct shift */

#define ODDSHIFT ((bplcon0 & 0x8000) ? oddhiscroll:oddscroll);
#define EVENSHIFT ((bplcon0 & 0x8000) ? evenhiscroll:evenscroll);

/* Macro for decoding 1 plane (8 pixels) */

#define P2C_SET(CHUNKY1,CHUNKY2,BDATA,DECODEARRAY) \
        CHUNKY1 = DECODEARRAY[BDATA][0]; \
        CHUNKY2 = DECODEARRAY[BDATA][1];

#define P2C_OR(CHUNKY1,CHUNKY2,BDATA,DECODEARRAY) \
        CHUNKY1 |= DECODEARRAY[BDATA][0]; \
        CHUNKY2 |= DECODEARRAY[BDATA][1];

/* Macro for reading a byte of bitplane data */

#define BDATA_READ(BDATA,BPTR) \
        BDATA = *(BPTR);

/* Macro for storing decoded data to line-arrays (8 pixels) */

#define LINE_STORE(CHUNKY1,CHUNKY2,LINEPTR) \
        *(LINEPTR) = CHUNKY1; \
        *(LINEPTR+1) = CHUNKY2;

/* Macro for oring decoded data to line-arrays (8 pixels) */

#define LINE_OR(CHUNKY1,CHUNKY2,LINEPTR) \
        *(LINEPTR) |= CHUNKY1; \
        *(LINEPTR+1) |= CHUNKY2;

/* Macro for handling the planar to chunky conversion for one byte of */
/* bitplane data */

#define BCONVERT(CHUNKY1,CHUNKY2,BDATA,BPTR,DECODEARRAY,CONVERTER) \
        BDATA_READ(BDATA,BPTR); \
        CONVERTER(CHUNKY1,CHUNKY2,BDATA,DECODEARRAY);

/* Hires displayed on lores routines */
/* Macro for decoding 1 plane (8 pixels) */

#define P2C_SET_320(CHUNKY,BDATA,DECODEARRAY) \
        CHUNKY = DECODEARRAY[BDATA]; 

#define P2C_OR_320(CHUNKY,BDATA,DECODEARRAY) \
        CHUNKY |= DECODEARRAY[BDATA]; 

/* Macro for storing decoded data to line-arrays (8 pixels) */

#define LINE_STORE_320(CHUNKY,LINEPTR) \
        *(LINEPTR) = CHUNKY; 

/* Macro for oring decoded data to line-arrays (8 pixels) */

#define LINE_OR_320(CHUNKY,LINEPTR) \
        *(LINEPTR) |= CHUNKY; 

/* Macro for handling the planar to chunky conversion for one byte of */
/* bitplane data */

#define BCONVERT_320(CHUNKY,BDATA,BPTR,DECODEARRAY,CONVERTER) \
        BDATA_READ(BDATA,BPTR); \
        CONVERTER(CHUNKY,BDATA,DECODEARRAY);


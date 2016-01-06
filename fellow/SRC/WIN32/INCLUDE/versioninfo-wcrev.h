#ifndef VERSIONINFO_H
#define VERSIONINFO_H

#define master 1
#if $WCBRANCH$ == 1
    #define FELLOWVERSION        "WinFellow v0.5.4 r$WCREV$ (Git-$WCCOMMITSHORT$)"
    #define FELLOWLONGVERSION    "WinFellow Amiga Emulator v0.5.4.$WCREV$ (Git-$WCCOMMITSHORT$)" 
#else
    #define FELLOWVERSION        "WinFellow v0.5.4 r$WCREV$ (Git-$WCBRANCH$-$WCCOMMITSHORT$)"
    #define FELLOWLONGVERSION    "WinFellow Amiga Emulator v0.5.4.$WCREV$ (Git-$WCBRANCH$-$WCCOMMITSHORT$)" 
#endif

#define FELLOWNUMERICVERSION "0.5.4.$WCREV$"

#endif

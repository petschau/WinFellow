# Microsoft Developer Studio Project File - Name="WinFellow" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WinFellow - Win32 Dx3 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WinFellow.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WinFellow.mak" CFG="WinFellow - Win32 Dx3 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WinFellow - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WinFellow - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "WinFellow - Win32 Dx3 Debug" (based on "Win32 (x86) Application")
!MESSAGE "WinFellow - Win32 Dx3 Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MT /GX /O2 /Ob2 /I "win32/include" /I "../include/msvc" /I "../../include" /I "../include" /I "../../uae/include" /I "../../xdms/include" /I "../../zlib/include" /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USE_DX5" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x414 /d "NDEBUG"
# ADD RSC /l 0x414 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib dinput.lib comctl32.lib dsound.lib /nologo /stack:0x200000 /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MTd /GX /ZI /Od /I "../include/msvc" /I "../../include" /I "../include" /I "../../uae/include" /I "../../xdms/include" /I "../../zlib/include" /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "USE_DX5" /FAcs /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x414 /d "_DEBUG"
# ADD RSC /l 0x417 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib dinput.lib comctl32.lib dsound.lib /nologo /stack:0x200000 /subsystem:windows /profile /map /debug /machine:I386

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinFellow___Win32_Dx3_Debug"
# PROP BASE Intermediate_Dir "WinFellow___Win32_Dx3_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Dx3Debug"
# PROP Intermediate_Dir "Dx3Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MTd /Gm /GX /Zi /Od /I "../include/msvc" /I "../../include" /I "../include" /I "../../uae/include" /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "USE_DX5" /FAcs /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G5 /MTd /GX /ZI /Od /I "../include/msvc" /I "../../include" /I "../include" /I "../../uae/include" /I "../../xdms/include" /I "../../zlib/include" /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "USE_DX3" /FAcs /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x414 /d "_DEBUG"
# ADD RSC /l 0x414 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib dinput.lib comctl32.lib dsound.lib /nologo /stack:0x200000 /subsystem:windows /profile /map /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib dinput.lib comctl32.lib dsound.lib /nologo /stack:0x200000 /subsystem:windows /profile /map /debug /machine:I386

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinFellow___Win32_Dx3_Release"
# PROP BASE Intermediate_Dir "WinFellow___Win32_Dx3_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dx3Release"
# PROP Intermediate_Dir "Dx3Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MT /GX /O2 /I "win32/include" /I "../include/msvc" /I "../../include" /I "../include" /I "../../uae/include" /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USE_DX5" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G5 /MT /GX /O2 /I "win32/include" /I "../include/msvc" /I "../../include" /I "../include" /I "../../uae/include" /I "../../xdms/include" /I "../../zlib/include" /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USE_DX3" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x414 /d "NDEBUG"
# ADD RSC /l 0x414 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib dinput.lib comctl32.lib dsound.lib /nologo /stack:0x200000 /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib winmm.lib dinput.lib comctl32.lib dsound.lib /nologo /stack:0x200000 /subsystem:windows /machine:I386

!ENDIF 

# Begin Target

# Name "WinFellow - Win32 Release"
# Name "WinFellow - Win32 Debug"
# Name "WinFellow - Win32 Dx3 Debug"
# Name "WinFellow - Win32 Dx3 Release"
# Begin Group "Assembly Files"

# PROP Default_Filter "*.s"
# Begin Source File

SOURCE=..\..\asm\blita.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\blita.s

"$(IntDir)\blita.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\blita.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\blita.s

"$(IntDir)\blita.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\blita.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\blita.s

"$(IntDir)\blita.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\blita.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\blita.s

"$(IntDir)\blita.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\blita.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\busa.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\busa.s

"$(IntDir)\busa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\busa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\busa.s

"$(IntDir)\busa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\busa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\busa.s

"$(IntDir)\busa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\busa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\busa.s

"$(IntDir)\busa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\busa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\ciaa.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\ciaa.s

"$(IntDir)\ciaa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\ciaa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\ciaa.s

"$(IntDir)\ciaa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\ciaa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\ciaa.s

"$(IntDir)\ciaa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\ciaa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\ciaa.s

"$(IntDir)\ciaa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\ciaa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\coppera.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\coppera.s

"$(IntDir)\coppera.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\coppera.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\coppera.s

"$(IntDir)\coppera.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\coppera.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\coppera.s

"$(IntDir)\coppera.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\coppera.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\coppera.s

"$(IntDir)\coppera.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\coppera.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\cpua.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\cpua.s

"$(IntDir)\cpua.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\cpua.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\cpua.s

"$(IntDir)\cpua.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\cpua.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\cpua.s

"$(IntDir)\cpua.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\cpua.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\cpua.s

"$(IntDir)\cpua.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\cpua.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\drawa.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\drawa.s

"$(IntDir)\drawa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\drawa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\drawa.s

"$(IntDir)\drawa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\drawa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\drawa.s

"$(IntDir)\drawa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\drawa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\drawa.s

"$(IntDir)\drawa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\drawa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\fhfilea.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\fhfilea.s

"$(IntDir)\fhfilea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\fhfilea.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\fhfilea.s

"$(IntDir)\fhfilea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\fhfilea.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\fhfilea.s

"$(IntDir)\fhfilea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\fhfilea.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\fhfilea.s

"$(IntDir)\fhfilea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\fhfilea.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\fmema.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\fmema.s

"$(IntDir)\fmema.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\fmema.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\fmema.s

"$(IntDir)\fmema.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\fmema.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\fmema.s

"$(IntDir)\fmema.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\fmema.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\fmema.s

"$(IntDir)\fmema.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\fmema.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\grapha.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\grapha.s

"$(IntDir)\grapha.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\grapha.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\grapha.s

"$(IntDir)\grapha.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\grapha.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\grapha.s

"$(IntDir)\grapha.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\grapha.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\grapha.s

"$(IntDir)\grapha.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\grapha.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\mmx.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\mmx.s

"$(IntDir)\mmx.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\mmx.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\mmx.s

"$(IntDir)\mmx.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\mmx.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\mmx.s

"$(IntDir)\mmx.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\mmx.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\mmx.s

"$(IntDir)\mmx.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\mmx.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\sounda.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\sounda.s

"$(IntDir)\sounda.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\sounda.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\sounda.s

"$(IntDir)\sounda.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\sounda.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\sounda.s

"$(IntDir)\sounda.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\sounda.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\sounda.s

"$(IntDir)\sounda.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\sounda.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\asm\spritea.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\asm\spritea.s

"$(IntDir)\spritea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\spritea.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\asm\spritea.s

"$(IntDir)\spritea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\spritea.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\asm\spritea.s

"$(IntDir)\spritea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\spritea.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\asm\spritea.s

"$(IntDir)\spritea.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\spritea.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Asm\Sse.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\Asm\Sse.s

"$(IntDir)\sse.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\sse.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\Asm\Sse.s

"$(IntDir)\sse.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\sse.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\Asm\Sse.s

"$(IntDir)\sse.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\sse.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\Asm\Sse.s

"$(IntDir)\sse.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\sse.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Assembly Include Files"

# PROP Default_Filter ""
# Begin Group "Macros"

# PROP Default_Filter "*.mac"
# Begin Source File

SOURCE=..\..\incasm\mac\blit.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\bus.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\cia.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\cpu.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\draw.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\drawmode.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\floppy.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\fmem.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\gameport.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\graph.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\kbd.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\nasm.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\plan2c.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\sound.mac
# End Source File
# Begin Source File

SOURCE=..\..\incasm\mac\sprite.mac
# End Source File
# End Group
# Begin Group "Functions"

# PROP Default_Filter "*.inc"
# Begin Source File

SOURCE=..\..\incasm\func\blit.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\bus.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\cia.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\copper.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\cpu.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\draw.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\fellow.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\floppy.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\fmem.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\gameport.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\graph.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\kbd.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\sound.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\func\sprite.inc
# End Source File
# End Group
# Begin Group "Data"

# PROP Default_Filter "*.inc"
# Begin Source File

SOURCE=..\..\incasm\data\blit.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\bus.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\cia.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\copper.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\cpu.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\draw.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\fellow.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\fhfile.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\floppy.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\fmem.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\gameport.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\graph.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\sound.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\sprite.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\data\timer.inc
# End Source File
# End Group
# Begin Group "Generic"

# PROP Default_Filter "*.inc"
# Begin Source File

SOURCE=..\..\incasm\generic\defs.inc
# End Source File
# Begin Source File

SOURCE=..\..\incasm\generic\sound.inc
# End Source File
# End Group
# End Group
# Begin Group "core C Files"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=..\..\c\blit.c
# End Source File
# Begin Source File

SOURCE=..\..\c\bus.c
# End Source File
# Begin Source File

SOURCE=..\..\c\cia.c
# End Source File
# Begin Source File

SOURCE=..\..\c\config.c
# End Source File
# Begin Source File

SOURCE=..\..\c\copper.c
# End Source File
# Begin Source File

SOURCE=..\..\c\cpu.c
# End Source File
# Begin Source File

SOURCE=..\..\c\cpudis.c
# End Source File
# Begin Source File

SOURCE=..\..\c\draw.c
# End Source File
# Begin Source File

SOURCE=..\..\c\fellow.c
# ADD CPP /GX-
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\c\ffilesys.c
# End Source File
# Begin Source File

SOURCE=..\..\c\fhfile.c
# End Source File
# Begin Source File

SOURCE=..\..\c\floppy.c
# End Source File
# Begin Source File

SOURCE=..\..\c\fmem.c
# End Source File
# Begin Source File

SOURCE=..\..\c\fsnavig.c
# End Source File
# Begin Source File

SOURCE=..\C\fsysamnt.c
# End Source File
# Begin Source File

SOURCE=..\..\c\gameport.c
# End Source File
# Begin Source File

SOURCE=..\..\c\graph.c
# End Source File
# Begin Source File

SOURCE=..\C\Ini.c
# End Source File
# Begin Source File

SOURCE=..\..\c\kbd.c
# End Source File
# Begin Source File

SOURCE=..\..\c\listtree.c
# End Source File
# Begin Source File

SOURCE=..\..\C\modrip.c
# End Source File
# Begin Source File

SOURCE=..\..\c\sound.c
# End Source File
# Begin Source File

SOURCE=..\..\c\sprite.c
# End Source File
# Begin Source File

SOURCE=..\..\c\wav.c
# End Source File
# Begin Source File

SOURCE=..\..\C\zlibwrap.c
# End Source File
# End Group
# Begin Group "core C Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE="C:\Program Files\Microsoft Visual Studio\Vc98\Include\Basetsd.h"
# End Source File
# Begin Source File

SOURCE=..\..\include\blit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\bus.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cia.h
# End Source File
# Begin Source File

SOURCE=..\..\include\config.h
# End Source File
# Begin Source File

SOURCE=..\..\include\copper.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cpu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cpudis.h
# End Source File
# Begin Source File

SOURCE=..\..\include\defs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\draw.h
# End Source File
# Begin Source File

SOURCE=..\Include\dxver.h
# End Source File
# Begin Source File

SOURCE=..\..\include\eventid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fellow.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ffilesys.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fhfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\floppy.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fmem.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fonts.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fsnavig.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gameport.h
# End Source File
# Begin Source File

SOURCE=..\..\include\graph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\graphm.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Ini.h
# End Source File
# Begin Source File

SOURCE=..\..\include\keycodes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\listtree.h
# End Source File
# Begin Source File

SOURCE=..\..\include\memorya.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mmx.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\modrip.h
# End Source File
# Begin Source File

SOURCE=..\include\msvc\portable.h
# End Source File
# Begin Source File

SOURCE=..\include\msvc\renaming.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sprite.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Sse.h
# End Source File
# Begin Source File

SOURCE=..\..\include\wav.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\zlibwrap.h
# End Source File
# End Group
# Begin Group "Win32 C Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Include\commoncontrol_wrap.h
# End Source File
# Begin Source File

SOURCE=..\include\fswrap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gfxdrv.h
# End Source File
# Begin Source File

SOURCE=..\..\include\joydrv.h
# End Source File
# Begin Source File

SOURCE=..\..\include\kbd.h
# End Source File
# Begin Source File

SOURCE=..\..\include\kbddrv.h
# End Source File
# Begin Source File

SOURCE=..\Include\kbdparser.h
# End Source File
# Begin Source File

SOURCE=..\Include\modrip_win32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mousedrv.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sound.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sounddrv.h
# End Source File
# Begin Source File

SOURCE=..\..\include\timer.h
# End Source File
# Begin Source File

SOURCE=..\include\wdbg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\wgui.h
# End Source File
# Begin Source File

SOURCE=..\include\windrv.h
# End Source File
# End Group
# Begin Group "Win32 Assembly Include Files"

# PROP Default_Filter ""
# Begin Group "Macro"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\incasm\mac\callconv.mac
# End Source File
# Begin Source File

SOURCE=..\incasm\mac\renaming.mac
# End Source File
# End Group
# End Group
# Begin Group "Win32 C Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\C\commoncontrol_wrap.c
# End Source File
# Begin Source File

SOURCE=..\C\fswrap.c
# End Source File
# Begin Source File

SOURCE=..\C\gfxdrv.c
# End Source File
# Begin Source File

SOURCE=..\C\joydrv.c
# End Source File
# Begin Source File

SOURCE=..\C\kbddrv.c
# End Source File
# Begin Source File

SOURCE=..\C\kbdparser.c
# End Source File
# Begin Source File

SOURCE=..\C\modrip_win32.c
# End Source File
# Begin Source File

SOURCE=..\C\mousedrv.c
# End Source File
# Begin Source File

SOURCE=..\C\sounddrv.c
# End Source File
# Begin Source File

SOURCE=..\C\sysinfo.c
# End Source File
# Begin Source File

SOURCE=..\c\timer.c
# End Source File
# Begin Source File

SOURCE=..\c\wdbg.c
# End Source File
# Begin Source File

SOURCE=..\c\wgui.c
# End Source File
# Begin Source File

SOURCE=..\c\winmain.c
# End Source File
# End Group
# Begin Group "UAE Filesystem Files"

# PROP Default_Filter ""
# Begin Group "UAE Filesystem C Files (Win32)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\uae\c\autoconf.c
# End Source File
# Begin Source File

SOURCE=..\uae\c\expansio.c
# End Source File
# Begin Source File

SOURCE=..\uae\c\filesys.c
# End Source File
# Begin Source File

SOURCE=..\Uae\C\filesys_win32.c
# End Source File
# Begin Source File

SOURCE=..\Uae\C\fsdb.c
# End Source File
# Begin Source File

SOURCE=..\Uae\C\fsdb_win32.c
# End Source File
# Begin Source File

SOURCE=..\uae\c\fsusage.c
# End Source File
# Begin Source File

SOURCE=..\uae\c\hardfile.c
# End Source File
# Begin Source File

SOURCE=..\uae\c\uaesupp.c
# End Source File
# End Group
# Begin Group "UAE Filesystem Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\uae\include\autoconf.h
# End Source File
# Begin Source File

SOURCE=..\..\uae\include\execlib.h
# End Source File
# Begin Source File

SOURCE=..\..\uae\include\filesys.h
# End Source File
# Begin Source File

SOURCE=..\..\Uae\Include\filesys_win32.h
# End Source File
# Begin Source File

SOURCE=..\..\Uae\Include\fsdb.h
# End Source File
# Begin Source File

SOURCE=..\..\uae\include\fsusage.h
# End Source File
# Begin Source File

SOURCE=..\..\uae\include\penguin.h
# End Source File
# Begin Source File

SOURCE=..\..\uae\include\uae2fell.h
# End Source File
# Begin Source File

SOURCE=..\..\uae\include\uaefsys.h
# End Source File
# End Group
# Begin Group "UAE Filesystem Assembly Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\uae\asm\uaesuppa.s

!IF  "$(CFG)" == "WinFellow - Win32 Release"

# Begin Custom Build
IntDir=.\Release
ProjDir=.
InputPath=..\..\uae\asm\uaesuppa.s

"$(IntDir)\uaesuppa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\uaesuppa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ -i$(ProjDir)\..\..\uae\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
ProjDir=.
InputPath=..\..\uae\asm\uaesuppa.s

"$(IntDir)\uaesuppa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\uaesuppa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ -i$(ProjDir)\..\..\uae\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Debug"

# Begin Custom Build
IntDir=.\Dx3Debug
ProjDir=.
InputPath=..\..\uae\asm\uaesuppa.s

"$(IntDir)\uaesuppa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\uaesuppa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ -i$(ProjDir)\..\..\uae\incasm\ $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WinFellow - Win32 Dx3 Release"

# Begin Custom Build
IntDir=.\Dx3Release
ProjDir=.
InputPath=..\..\uae\asm\uaesuppa.s

"$(IntDir)\uaesuppa.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\uaesuppa.obj -f win32 -i$(ProjDir)\..\..\incasm\               -i$(ProjDir)\..\incasm\ -i$(ProjDir)\..\..\uae\incasm\ $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "UAE Filesystem Assembly Includ"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\uae\incasm\func\uaesupp.inc
# End Source File
# End Group
# End Group
# Begin Group "ADF Compression Files"

# PROP Default_Filter ""
# Begin Group "xDMS C Files"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\..\xdms\C\crc_csum.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\getbits.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\maketbl.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\pfile.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\tables.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\u_deep.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\u_heavy.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\u_init.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\u_medium.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\u_quick.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\u_rle.c
# End Source File
# Begin Source File

SOURCE=..\..\xdms\C\xdms.c
# End Source File
# End Group
# Begin Group "xDMS Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=..\..\xdms\include\cdata.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\crc_csum.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\getbits.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\maketbl.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\pfile.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\tables.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\u_deep.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\u_heavy.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\u_init.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\u_medium.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\u_quick.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\u_rle.h
# End Source File
# Begin Source File

SOURCE=..\..\xdms\include\xdms.h
# End Source File
# End Group
# Begin Group "zlib C Files"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=..\..\zlib\C\adler32.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\compress.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\crc32.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\deflate.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\gzio.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\infblock.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\infcodes.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\inffast.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\inflate.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\infutil.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\trees.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\C\zutil.c
# End Source File
# End Group
# Begin Group "zlib Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=..\..\zlib\include\deflate.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\infblock.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\infcodes.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\inffast.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\infutil.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\trees.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\zconf.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\zlib.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\include\zutil.h
# End Source File
# End Group
# End Group
# Begin Group "bitmaps"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\disk_led_disabled_cool.bmp
# End Source File
# Begin Source File

SOURCE=.\disk_led_off_cool.bmp
# End Source File
# Begin Source File

SOURCE=.\disk_led_on_cool.bmp
# End Source File
# Begin Source File

SOURCE=.\power_led_off_cool.bmp
# End Source File
# Begin Source File

SOURCE=.\power_led_on_cool.bmp
# End Source File
# Begin Source File

SOURCE=.\winfellow_logo_small.bmp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gui_debugger.h
# End Source File
# Begin Source File

SOURCE=.\GUI_debugger.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\gui_general.h
# End Source File
# Begin Source File

SOURCE=.\GUI_general.rc
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x417
# End Source File
# End Group
# Begin Group "icons"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\icons\cpu.ico
# End Source File
# Begin Source File

SOURCE=..\icons\dfloppy.ico
# End Source File
# Begin Source File

SOURCE=..\icons\display.ico
# End Source File
# Begin Source File

SOURCE=..\icons\filesystem.ico
# End Source File
# Begin Source File

SOURCE=..\icons\floppy.ico
# End Source File
# Begin Source File

SOURCE=..\icons\harddrives.ico
# End Source File
# Begin Source File

SOURCE=..\icons\sound.ico
# End Source File
# Begin Source File

SOURCE=.\winfellow.ico
# End Source File
# End Group
# End Target
# End Project

############################################################################################
#      NSIS Installation Script created by NSIS Quick Setup Script Generator v1.09.18
#               Entirely Edited with NullSoft Scriptable Installation System
#              by Vlasis K. Barkas aka Red Wine red_wine@freemail.gr Sep 2006
############################################################################################

!define APP_NAME "WinFellow"
!define COMP_NAME "The WinFellow Team"
!define WEB_SITE "http://petschau.github.io/WinFellow"
!define VERSION "${FELLOWVERSION}"
!define COPYRIGHT "© 1996-2023 under the GNU General Public License"
!define DESCRIPTION "WinFellow Amiga Emulator"
!define LICENSE_TXT "gpl-2.0.txt"
!define INSTALLER_NAME "WinFellow_v${FELLOWVERSION}.exe"
!define MAIN_APP_EXE "WinFellow.exe"
!define INSTALL_TYPE "SetShellVarContext all"
!define REG_ROOT "HKLM"
!define REG_APP_PATH "Software\Microsoft\Windows\CurrentVersion\App Paths\${MAIN_APP_EXE}"
!define UNINSTALL_PATH "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

!define REG_START_MENU "Start Menu Folder"

var SM_Folder

######################################################################

VIProductVersion  "${VERSION}"
VIAddVersionKey "ProductName"  "${APP_NAME}"
VIAddVersionKey "CompanyName"  "${COMP_NAME}"
VIAddVersionKey "LegalCopyright"  "${COPYRIGHT}"
VIAddVersionKey "FileDescription"  "${DESCRIPTION}"
VIAddVersionKey "FileVersion"  "${VERSION}"

######################################################################

SetCompressor ZLIB
Name "${APP_NAME}"
Caption "${APP_NAME}"
OutFile "$%TEMP%\${INSTALLER_NAME}"
BrandingText "${APP_NAME}"
XPStyle on
InstallDirRegKey "${REG_ROOT}" "${REG_APP_PATH}" ""

######################################################################

!include LogicLib.nsh
!include x64.nsh

Function .onInit
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
  "UninstallString"
  StrCmp $R0 "" done

  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${APP_NAME} is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort

;Run the uninstaller
uninst:
    ClearErrors
		Var /GLOBAL ReturnCode
    ExecWait '"$R0" /S' $ReturnCode
		
		# Error handler
		${If} $ReturnCode != 0 
		${AndIf} $ReturnCode != 3010
			MessageBox MB_OK|MB_ICONSTOP "${APP_NAME} uninstallation failed, aborting. "
			Abort
		${EndIf}
		
done:
	${If} ${RunningX64}
		StrCpy $InstDir "$PROGRAMFILES64\${APP_NAME}"
	${Else}
		StrCpy $InstDir "$PROGRAMFILES32\${APP_NAME}"
	${EndIf}
FunctionEnd

######################################################################

!include "MUI.nsh"

!define MUI_ICON "..\MSVC\WinFellow.ico"

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

!insertmacro MUI_PAGE_WELCOME

!ifdef LICENSE_TXT
!insertmacro MUI_PAGE_LICENSE "${LICENSE_TXT}"
!endif

!insertmacro MUI_PAGE_DIRECTORY

!ifdef REG_START_MENU
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "WinFellow"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${REG_ROOT}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${UNINSTALL_PATH}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${REG_START_MENU}"
!insertmacro MUI_PAGE_STARTMENU Application $SM_Folder
!endif

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\${MAIN_APP_EXE}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

######################################################################

Section -MainProgram
${INSTALL_TYPE}
SetOverwrite ifnewer
SetOutPath "$INSTDIR"
File "$%TEMP%\WinFellow_v${FELLOWVERSION}\ChangeLog.txt"
File "$%TEMP%\WinFellow_v${FELLOWVERSION}\gpl-2.0.pdf"
File "$%TEMP%\WinFellow_v${FELLOWVERSION}\WinFellow User Manual.pdf"
${If} ${RunningX64}
    File /oname=WinFellow.exe "$%TEMP%\WinFellow_v${FELLOWVERSION}\WinFellow-x64.exe"
    File /oname=WinFellow.pdb "$%TEMP%\WinFellow_v${FELLOWVERSION}\WinFellow-x64.pdb"
${Else}
    File /oname=WinFellow.exe "$%TEMP%\WinFellow_v${FELLOWVERSION}\WinFellow-x86.exe"
    File /oname=WinFellow.pdb "$%TEMP%\WinFellow_v${FELLOWVERSION}\WinFellow-x86.pdb"
${EndIf}
SetOutPath "$INSTDIR\Presets"
File /r "$%TEMP%\WinFellow_v${FELLOWVERSION}\Presets\*"
SetOutPath "$INSTDIR\Utilities"
File /r "$%TEMP%\WinFellow_v${FELLOWVERSION}\Utilities\*"
SectionEnd

######################################################################

Section -Icons_Reg
SetOutPath "$INSTDIR"
WriteUninstaller "$INSTDIR\uninstall.exe"

!ifdef REG_START_MENU
!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
CreateDirectory "$SMPROGRAMS\$SM_Folder"
CreateShortCut "$SMPROGRAMS\$SM_Folder\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
CreateShortCut "$SMPROGRAMS\$SM_Folder\WinFellow User Manual.lnk" "$INSTDIR\WinFellow User Manual.pdf"
CreateShortCut "$SMPROGRAMS\$SM_Folder\ChangeLog.lnk" "$INSTDIR\ChangeLog.txt"
CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
!ifdef WEB_SITE
WriteIniStr "$INSTDIR\${APP_NAME} website.url" "InternetShortcut" "URL" "${WEB_SITE}"
CreateShortCut "$SMPROGRAMS\$SM_Folder\${APP_NAME} Website.lnk" "$INSTDIR\${APP_NAME} website.url"
!endif
!insertmacro MUI_STARTMENU_WRITE_END
!endif

!ifndef REG_START_MENU
CreateDirectory "$SMPROGRAMS\WinFellow"
CreateShortCut "$SMPROGRAMS\WinFellow\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
!ifdef WEB_SITE
WriteIniStr "$INSTDIR\${APP_NAME} website.url" "InternetShortcut" "URL" "${WEB_SITE}"
CreateShortCut "$SMPROGRAMS\WinFellow\${APP_NAME} Website.lnk" "$INSTDIR\${APP_NAME} website.url"
!endif
!endif

WriteRegStr ${REG_ROOT} "${REG_APP_PATH}" "" "$INSTDIR\${MAIN_APP_EXE}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "DisplayName" "${APP_NAME}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "UninstallString" "$INSTDIR\uninstall.exe"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "DisplayIcon" "$INSTDIR\${MAIN_APP_EXE}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "DisplayVersion" "${VERSION}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "Publisher" "${COMP_NAME}"

!ifdef WEB_SITE
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "URLInfoAbout" "${WEB_SITE}"
!endif
SectionEnd

######################################################################

Section Uninstall
${INSTALL_TYPE}
Delete "$INSTDIR\ChangeLog.txt"
Delete "$INSTDIR\gpl-2.0.pdf"
Delete "$INSTDIR\WinFellow User Manual.pdf"
Delete "$INSTDIR\WinFellow.pdb"
Delete "$INSTDIR\WinFellow.exe"
Delete "$INSTDIR\uninstall.exe"
RMDir /r "$INSTDIR\Presets"
RMDir /r "$INSTDIR\Utilities"
!ifdef WEB_SITE
Delete "$INSTDIR\${APP_NAME} website.url"
!endif

RmDir "$INSTDIR"

!ifdef REG_START_MENU
!insertmacro MUI_STARTMENU_GETFOLDER "Application" $SM_Folder
Delete "$SMPROGRAMS\$SM_Folder\${APP_NAME}.lnk"
Delete "$SMPROGRAMS\$SM_Folder\WinFellow User Manual.lnk"
Delete "$SMPROGRAMS\$SM_Folder\ChangeLog.lnk"

!ifdef WEB_SITE
Delete "$SMPROGRAMS\$SM_Folder\${APP_NAME} Website.lnk"
!endif
Delete "$DESKTOP\${APP_NAME}.lnk"

RmDir "$SMPROGRAMS\$SM_Folder"
!endif

!ifndef REG_START_MENU
Delete "$SMPROGRAMS\WinFellow\${APP_NAME}.lnk"
!ifdef WEB_SITE
Delete "$SMPROGRAMS\WinFellow\${APP_NAME} Website.lnk"
!endif
Delete "$DESKTOP\${APP_NAME}.lnk"

RmDir "$SMPROGRAMS\WinFellow"
!endif

DeleteRegKey ${REG_ROOT} "${REG_APP_PATH}"
DeleteRegKey ${REG_ROOT} "${UNINSTALL_PATH}"
SectionEnd

######################################################################

/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* Windows common control wrap code                                           */
/* Author: Worfje (worfje@gmx.net)                                            */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

#include <windows.h>
#include <windowsx.h>
#include <windef.h>
#include <string.h>
#include <stdio.h>
#include <commctrl.h>

#include "defs.h"
#include "commoncontrol_wrap.h"

#include "windrv.h"

void ccwButtonSetCheck(HWND windowHandle, int controlIdentifier) {

	Button_SetCheck(GetDlgItem(windowHandle, controlIdentifier), TRUE);
}

void ccwButtonUncheck(HWND windowHandle, int controlIdentifier) {

	Button_SetCheck(GetDlgItem(windowHandle, controlIdentifier), FALSE);
}

void ccwButtonCheckConditional(HWND windowHandle, int controlIdentifier, BOOLE check) {

	Button_SetCheck(GetDlgItem(windowHandle, controlIdentifier), check);
}

BOOLE ccwButtonGetCheck(HWND windowHandle, int controlIdentifier) {
											 
	return Button_GetCheck(GetDlgItem(windowHandle, controlIdentifier));
}

void ccwButtonEnableConditional(HWND windowHandle, int controlIdentifier, BOOLE enable) {

	Button_Enable(GetDlgItem(windowHandle, controlIdentifier), enable);
}

void ccwButtonDisable(HWND windowHandle, int controlIdentifier) {

	ccwButtonEnableConditional(windowHandle, controlIdentifier, FALSE);
}

void ccwButtonEnable(HWND windowHandle, int controlIdentifier) {

	ccwButtonEnableConditional(windowHandle, controlIdentifier, TRUE);
}

void ccwSliderSetRange(HWND windowHandle, int controlIdentifier, ULO minPos, ULO maxPos) {
  
	SendMessage(GetDlgItem(windowHandle, controlIdentifier), TBM_SETRANGE, TRUE, (LPARAM) MAKELONG(minPos, maxPos));
}

ULO ccwComboBoxGetCurrentSelection(HWND windowHandle, int controlIdentifier) {
	
	return ComboBox_GetCurSel(GetDlgItem(windowHandle, controlIdentifier));
}

void ccwComboBoxSetCurrentSelection(HWND windowHandle, int controlIdentifier, ULO index) {
	
	ComboBox_SetCurSel(GetDlgItem(windowHandle, controlIdentifier), index);
}

void ccwComboBoxAddString(HWND windowHandle, int controlIdentifier, STR *text) {

	ComboBox_AddString(GetDlgItem(windowHandle, controlIdentifier), text);
}

ULO ccwSliderGetPosition(HWND windowHandle, int controlIdentifier) {

	return SendMessage(GetDlgItem(windowHandle, controlIdentifier), TBM_GETPOS, 0, 0);
}

void ccwSliderSetPosition(HWND windowHandle, int controlIdentifier, LONG position) {

	SendMessage(GetDlgItem(windowHandle, controlIdentifier), TBM_SETPOS, TRUE, (LPARAM) position);
}

void ccwStaticSetText(HWND windowHandle, int controlIdentifier, STR *text) {

	Static_SetText(GetDlgItem(windowHandle, controlIdentifier), text);
}

void ccwEditSetText(HWND windowHandle, int controlIdentifier, STR *text) {

	Edit_SetText(GetDlgItem(windowHandle, controlIdentifier), text);
}

void ccwEditGetText(HWND windowHandle, int controlIdentifier, STR *text, ULO n) {

	Edit_GetText(GetDlgItem(windowHandle, controlIdentifier), text, n);
}

void ccwEditEnableConditional(HWND windowHandle, int controlIdentifier, BOOLE enable) {

	Edit_Enable(GetDlgItem(windowHandle, controlIdentifier), enable);
}

void ccwSetImageConditional(HWND windowHandle, int controlIdentifier, HBITMAP bitmapFirst, HBITMAP bitmapSecond, BOOLE setFirst) {

	if (setFirst) {
		SendMessage(GetDlgItem(windowHandle, controlIdentifier), STM_SETIMAGE, IMAGE_BITMAP, 
		(LPARAM) bitmapFirst);
	} else {
		SendMessage(GetDlgItem(windowHandle, controlIdentifier), STM_SETIMAGE, IMAGE_BITMAP, 
		(LPARAM) bitmapSecond);
	}
}


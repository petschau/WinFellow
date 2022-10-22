/*=========================================================================*/
/* Fellow                                                                  */
/* Windows common control wrap code                                        */
/* Author: Worfje (worfje@gmx.net)                                         */
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

#include <windows.h>
#include <windowsx.h>
#include <windef.h>
#include <stdio.h>
#include <commctrl.h>

#include "fellow/api/defs.h"
#include "commoncontrol_wrap.h"

//#include "windrv.h"

void ccwButtonSetCheck(HWND windowHandle, int controlIdentifier)
{
  Button_SetCheck(GetDlgItem(windowHandle, controlIdentifier), BST_CHECKED);
}

void ccwButtonUncheck(HWND windowHandle, int controlIdentifier)
{
  Button_SetCheck(GetDlgItem(windowHandle, controlIdentifier), BST_UNCHECKED);
}

void ccwButtonCheckConditional(HWND windowHandle, int controlIdentifier, BOOLE check)
{
  ccwButtonCheckConditionalBool(windowHandle, controlIdentifier, !!check);
}

void ccwButtonCheckConditionalBool(HWND windowHandle, int controlIdentifier, bool check)
{
  if (check)
    ccwButtonSetCheck(windowHandle, controlIdentifier);
  else
    ccwButtonUncheck(windowHandle, controlIdentifier);
}

BOOLE ccwButtonGetCheck(HWND windowHandle, int controlIdentifier)
{
  return Button_GetCheck(GetDlgItem(windowHandle, controlIdentifier)) == BST_CHECKED;
}

bool ccwButtonGetCheckBool(HWND windowHandle, int controlIdentifier)
{
  return Button_GetCheck(GetDlgItem(windowHandle, controlIdentifier)) == BST_CHECKED;
}

void ccwButtonEnableConditional(HWND windowHandle, int controlIdentifier, BOOLE enable)
{
  Button_Enable(GetDlgItem(windowHandle, controlIdentifier), enable);
}

void ccwButtonDisable(HWND windowHandle, int controlIdentifier)
{
  ccwButtonEnableConditional(windowHandle, controlIdentifier, FALSE);
}

void ccwButtonEnable(HWND windowHandle, int controlIdentifier)
{
  ccwButtonEnableConditional(windowHandle, controlIdentifier, TRUE);
}

void ccwSliderSetRange(HWND windowHandle, int controlIdentifier, ULO minPos, ULO maxPos)
{
  SendMessage(GetDlgItem(windowHandle, controlIdentifier), TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(minPos, maxPos));
}

ULO ccwComboBoxGetCurrentSelection(HWND windowHandle, int controlIdentifier)
{
  return ComboBox_GetCurSel(GetDlgItem(windowHandle, controlIdentifier));
}

void ccwComboBoxSetCurrentSelection(HWND windowHandle, int controlIdentifier, ULO index)
{
  ComboBox_SetCurSel(GetDlgItem(windowHandle, controlIdentifier), index);
}

void ccwComboBoxAddString(HWND windowHandle, int controlIdentifier, STR *text)
{
  ComboBox_AddString(GetDlgItem(windowHandle, controlIdentifier), text);
}

ULO ccwSliderGetPosition(HWND windowHandle, int controlIdentifier)
{
  return (ULO)SendMessage(GetDlgItem(windowHandle, controlIdentifier), TBM_GETPOS, 0, 0);
}

void ccwSliderSetPosition(HWND windowHandle, int controlIdentifier, LONG position)
{
  SendMessage(GetDlgItem(windowHandle, controlIdentifier), TBM_SETPOS, TRUE, (LPARAM)position);
}

void ccwSliderEnable(HWND windowHandle, int controlIdentifier, BOOL enable)
{
  EnableWindow(GetDlgItem(windowHandle, controlIdentifier), enable);
}

void ccwSliderEnableBool(HWND windowHandle, int controlIdentifier, bool enable)
{
  EnableWindow(GetDlgItem(windowHandle, controlIdentifier), enable ? 1 : 0);
}

void ccwStaticSetText(HWND windowHandle, int controlIdentifier, STR *text)
{
  Static_SetText(GetDlgItem(windowHandle, controlIdentifier), text);
}

void ccwEditSetText(HWND windowHandle, int controlIdentifier, const STR *text)
{
  Edit_SetText(GetDlgItem(windowHandle, controlIdentifier), text);
}

void ccwEditGetText(HWND windowHandle, int controlIdentifier, STR *text, ULO n)
{
  Edit_GetText(GetDlgItem(windowHandle, controlIdentifier), text, n);
}

void ccwEditEnableConditional(HWND windowHandle, int controlIdentifier, BOOLE enable)
{
  Edit_Enable(GetDlgItem(windowHandle, controlIdentifier), enable);
}

void ccwEditEnableConditionalBool(HWND windowHandle, int controlIdentifier, bool enable)
{
  Edit_Enable(GetDlgItem(windowHandle, controlIdentifier), enable ? 1 : 0);
}

void ccwSetImageConditional(HWND windowHandle, int controlIdentifier, HBITMAP bitmapFirst, HBITMAP bitmapSecond, BOOLE setFirst)
{
  SendMessage(GetDlgItem(windowHandle, controlIdentifier), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(setFirst ? bitmapFirst : bitmapSecond));
}

void ccwMenuCheckedSetConditional(HWND windowHandle, int menuIdentifier, BOOLE enable)
{
  HMENU hMenu = GetMenu(windowHandle);

  MENUITEMINFO mii = {0};
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_STATE;

  GetMenuItemInfo(hMenu, menuIdentifier, FALSE, &mii);

  mii.fState |= enable ? MFS_CHECKED : MFS_UNCHECKED;
  SetMenuItemInfo(hMenu, menuIdentifier, FALSE, &mii);
}

BOOLE ccwMenuCheckedToggle(HWND windowHandle, int menuIdentifier)
{
  return ccwMenuCheckedToggleBool(windowHandle, menuIdentifier) == true;
}

bool ccwMenuCheckedToggleBool(HWND windowHandle, int menuIdentifier)
{
  HMENU hMenu = GetMenu(windowHandle);
  bool ischecked, result;

  MENUITEMINFO mii = {0};
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_STATE;

  GetMenuItemInfo(hMenu, menuIdentifier, FALSE, &mii);
  ischecked = mii.fState & MFS_CHECKED;
  result = !ischecked;

  CheckMenuItem(hMenu, menuIdentifier, result ? MFS_CHECKED : MFS_UNCHECKED);

  return result;
}
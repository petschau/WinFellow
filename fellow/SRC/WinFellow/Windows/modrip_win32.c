/*=========================================================================*/
/* Fellow                                                                  */
/*                                                                         */
/* OS-dependant parts of the module ripper - Windows GUI code              */
/* written by Torsten Enderling (carfesh@gmx.net)                          */
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

/* fellow includes */
#include "defs.h"

/* own includes */
#include <windows.h>
#include "modrip.h"
#include "modrip_win32.h"
#include "wgui.h"
#include "ini.h"

static HWND modrip_hWnd;
extern HWND wdbg_hDialog;

/*========================================================*/
/* GUI initializations                                    */
/* for windows this just needs to fetch the window handle */
/* from the debug module                                  */
/*========================================================*/

BOOLE modripGuiInitialize()
{
  modrip_hWnd = wdbg_hDialog;
  return TRUE;
}

/*==================================================*/
/* Instruct the GUI to show that the ripper is busy */
/* for windows this sets a busy cursor              */
/*==================================================*/

void modripGuiSetBusy()
{
  HCURSOR BusyCursor = LoadCursor(nullptr, MAKEINTRESOURCE(IDC_WAIT));
  if (BusyCursor) SetCursor(BusyCursor);
}

/*=======================================================*/
/* The GUI may show that the ripper isn't active anymore */
/* windows: set cursor to normal again                   */
/*=======================================================*/

void modripGuiUnSetBusy()
{
  HCURSOR NormalCursor = LoadCursor(nullptr, MAKEINTRESOURCE(IDC_ARROW));
  if (NormalCursor) SetCursor(NormalCursor);
}

/*=========================================================*/
/* The detection found a module and wants to save it       */
/* Show information about the module and return the user's */
/* decision whether to save it or not                      */
/*=========================================================*/

BOOLE modripGuiSaveRequest(struct ModuleInfo *info, MemoryAccessFunc func)
{
  char message[MODRIP_TEMPSTRLEN];
  char tempstr[MODRIP_TEMPSTRLEN];

  if (!info) return FALSE;

  sprintf(message, "Module found:\n");

  if (info->start)
  {
    sprintf(tempstr, "Location: 0x%06X\n", info->start);
    strcat(message, tempstr);
    if (info->end)
    {
      sprintf(tempstr, "Size: %u Bytes\n", info->end - info->start);
      strcat(message, tempstr);
    }
  }

  if (*(info->typedesc))
  {
    sprintf(tempstr, "Type: %s\n", info->typedesc);
    strcat(message, tempstr);
  }

  if (*(info->typesig))
  {
    sprintf(tempstr, "Signature: %s\n", info->typesig);
    strcat(message, tempstr);
  }

  if (*(info->modname))
  {
    sprintf(tempstr, "Module name: %s\n", info->modname);
    strcat(message, tempstr);
  }

  if (info->maxpattern)
  {
    sprintf(tempstr, "Patterns used: %u\n", info->maxpattern);
    strcat(message, tempstr);
  }

  if (info->channels)
  {
    sprintf(tempstr, "Channels used: %u\n", info->channels);
    strcat(message, tempstr);
  }

  if (*(info->filename))
  {
    sprintf(tempstr, "\nSave module as %s?", info->filename);
    strcat(message, tempstr);
  }
  else
  {
    strcat(message, "\nThe detection routine didn't provide a filename.\n");
    strcat(message, "Please contact the developers.");
  }

  if (MessageBox(modrip_hWnd, message, "Module found.", MB_YESNO | MB_ICONQUESTION) == IDYES)
  {
    if (wguiSaveFile(modrip_hWnd, info->filename, MODRIP_TEMPSTRLEN, "Save Module As:", FSEL_MOD))
    {
      if (!modripSaveMem(info, func))
      {
        modripGuiErrorSave(info);
        return FALSE;
      }
      else
      {
        iniSetLastUsedModDir(wgui_ini, (char *)wguiExtractPath(info->filename));
        return TRUE;
      }
    }
    else
      return FALSE;
  }
  return TRUE;
}

/*=====================================================*/
/* show information about an occured error upon saving */
/*=====================================================*/

void modripGuiErrorSave(struct ModuleInfo *info)
{
  char message[MODRIP_TEMPSTRLEN];

  if (!info) return;

  sprintf(message, "The module %s could not be saved.", info->filename);
  MessageBox(modrip_hWnd, message, "Error.", MB_OK | MB_ICONEXCLAMATION);
}

/*==========================================================*/
/* ask the user whether to scan the emulated Amiga's memory */
/* for modules                                              */
/*==========================================================*/

BOOLE modripGuiRipMemory()
{
  char message[MODRIP_TEMPSTRLEN];

  sprintf(message, "Do you want to scan the memory for modules?");
  int result = MessageBox(modrip_hWnd, message, "Memory scan.", MB_YESNO | MB_ICONQUESTION);
  return (result == IDYES);
}

/*=====================================================*/
/* Ask the user whether he wants to scan drive driveNo */
/*=====================================================*/

BOOLE modripGuiRipFloppy(int driveNo)
{
  char message[MODRIP_TEMPSTRLEN];

  if ((0 <= driveNo) && (driveNo < 4))
  {
    sprintf(message, "A floppy is inserted in drive DF%d and ", driveNo);
    strcat(message, "may be scanned for modules.\n");
    strcat(message, "Note that scanning a floppy will usually result in a ");
    strcat(message, "damaged module when scanning AmigaDOS formatted floppies.\n\n");
    strcat(message, "Do you want to do so?");
    int result = MessageBox(modrip_hWnd, message, "Drive scan possible.", MB_YESNO | MB_ICONQUESTION);
    return (result == IDYES);
  }
  else
    return FALSE;
}

/*==========================================*/
/* Clean up whatever needs to be cleaned up */
/* eventually show a final message          */
/*==========================================*/

void modripGuiUnInitialize()
{
  MessageBox(modrip_hWnd, "Module Ripper finished.", "Finished.", MB_OK | MB_ICONINFORMATION);
}

/*==========================*/
/* Display an error message */
/*==========================*/

void modripGuiError(char *message)
{
  MessageBox(modrip_hWnd, message, "Mod-Ripper Error.", MB_OK | MB_ICONSTOP);
}

/*==============================================*/
/* ask the user whether to dump the chip memory */
/*==============================================*/

BOOLE modripGuiDumpChipMem()
{
  char message[MODRIP_TEMPSTRLEN];

  sprintf(message, "You are about to dump the memory into the files \"chip.mem\",");
  strcat(message, " \"fast.mem\" and \"bogo.mem\".");
  strcat(message, " This feature can be used to run external module rippers over them.");
  strcat(message, " Do you really want to do that?");
  int result = MessageBox(modrip_hWnd, message, "Memory scan.", MB_YESNO | MB_ICONQUESTION);
  return (result == IDYES);
}

/*=====================================*/
/* run Pro-Wizard over the saved file? */
/*=====================================*/
BOOLE modripGuiRunProWiz()
{
  char message[MODRIP_TEMPSTRLEN];

  sprintf(message, "You have Pro-Wizard installed in WinFellow's directory.");
  strcat(message, " Do you want to run it over the saved chip memory file?");
  int result = MessageBox(modrip_hWnd, message, "Memory scan.", MB_YESNO | MB_ICONQUESTION);
  return (result == IDYES);
}

/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* OS-dependant parts of the module ripper - Windows GUI code                 */
/* Author: Torsten Enderling (carfesh@gmx.net)                                */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/

/* fellow includes */
#include "defs.h"

/* own includes */
#include <windows.h>
#include "modrip.h"


static HWND modrip_hWnd;
extern HWND wdbg_hDialog;

/*========================================================*/
/* GUI initializations                                    */
/* for windows this just needs to fetch the window handle */
/* from the debug module                                  */
/*========================================================*/

BOOLE modripGuiInitialize(void)
{
	modrip_hWnd = wdbg_hDialog;
}

/*==================================================*/
/* Instruct the GUI to show that the ripper is busy */
/* for windows this sets a busy cursor              */
/*==================================================*/

void modripGuiSetBusy(void)
{
  HCURSOR BusyCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
  if (BusyCursor) SetCursor(BusyCursor);
}

/*=======================================================*/
/* The GUI may show that the ripper isn't active anymore */
/* windows: set cursor to normal again                   */
/*=======================================================*/

void modripGuiUnSetBusy(void)
{
  HCURSOR NormalCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
  if (NormalCursor) SetCursor(NormalCursor);
}

/*=========================================================*/
/* The detection found a module and wants to save it       */
/* Show information about the module and return the user's */
/* decision whether to save it or not                      */
/*=========================================================*/

BOOLE modripGuiSaveRequest(struct ModuleInfo *info)
{
  char message[MODRIP_TEMPSTRLEN];
  char tempstr[MODRIP_TEMPSTRLEN];
  int result;

  if(!info) return FALSE;

  sprintf(message, "Module found:\n");
  
  if(info->start) {
    sprintf(tempstr, "Location: 0x%06X\n", info->start);
	strcat(message, tempstr);
	if(info->end) {
	  sprintf(tempstr, "Size: %u Bytes\n", info->end - info->start);
      strcat(message, tempstr);
	}
  }
  
  if(*(info->typedesc)) {
    sprintf(tempstr, "Type: %s\n", info->typedesc);
	strcat(message, tempstr);
  }

  if(*(info->typesig)) {
    sprintf(tempstr, "Signature: %s\n", info->typesig);
	strcat(message, tempstr);
  }

  if(*(info->modname)) {
    sprintf(tempstr, "Module name: %s\n", info->modname);
    strcat(message, tempstr);
  }

  if(info->maxpattern) {
    sprintf(tempstr, "Patterns used: %u\n", info->maxpattern);
    strcat(message, tempstr);
  }

  if(info->channels) {
    sprintf(tempstr, "Channels used: %u\n", info->channels);
    strcat(message, tempstr);
  }
  
  if(*(info->filename)) {
    sprintf(tempstr, "\nSave module as %s?", info->filename);
    strcat(message, tempstr);
  }
  else {
    strcat(message, "\nThe detection routine didn't provide a filename.\n");
	strcat(message, "Please contact the developers.");
  }
	
  result = MessageBox(modrip_hWnd, message, "Module found.", MB_YESNO |
    MB_ICONQUESTION);
  return(result == IDYES);
}

/*=====================================================*/
/* show information about an occured error upon saving */
/*=====================================================*/

void modripGuiErrorSave(struct ModuleInfo *info)
{
  char message[MODRIP_TEMPSTRLEN];

  if(!info) return;

  sprintf(message, "The module %s could not be saved.", info->filename);
  MessageBox(modrip_hWnd, message, "Error.", MB_OK | MB_ICONEXCLAMATION);
}

/*==========================================================*/
/* ask the user whether to scan the emulated Amiga's memory */
/* for modules                                              */
/*==========================================================*/

BOOLE modripGuiRipMemory(void)
{
  char message[MODRIP_TEMPSTRLEN];
  int result;

  sprintf(message, "Do you want to scan the memory for modules?");
  result = MessageBox(modrip_hWnd, message, "Memory scan.", MB_YESNO |
    MB_ICONQUESTION);
  return(result == IDYES);
}

/*=====================================================*/
/* Ask the user whether he wants to scan drive driveNo */
/*=====================================================*/

BOOLE modripGuiRipFloppy(int driveNo)
{
  char message[MODRIP_TEMPSTRLEN];
  int result;

  if((0 <= driveNo) && (driveNo < 4)) {
    sprintf(message, "A floppy is inserted in drive DF%d and ", driveNo);
	strcat(message, "may be scanned for modules.\n");
	strcat(message, "Note that scanning a floppy will usually result in a ");
	strcat(message, "damaged module when scanning AmigaDOS formatted floppies.\n\n");
	strcat(message, "Do you want to do so?");
    result = MessageBox(modrip_hWnd, message, "Drive scan possible.", MB_YESNO |
	  MB_ICONQUESTION);
	return(result == IDYES);
  }
  else
    return FALSE;
}

/*==========================================*/
/* Clean up whatever needs to be cleaned up */
/* eventually show a final message          */
/*==========================================*/

void modripGuiUnInitialize(void)
{
  MessageBox(modrip_hWnd, "Module Ripper finished.", "Finished.", 
    MB_OK | MB_ICONINFORMATION);
}

/*==========================*/
/* Display an error message */
/*==========================*/

void modripGuiError(char *message)
{
  MessageBox(modrip_hWnd, message, "Mod-Ripper Error.", MB_OK |
    MB_ICONSTOP);
}
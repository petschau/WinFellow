#ifndef COMMONCONTROLWRAP_H
#define COMMONCONTROLWRAP_H

void ccwButtonSetCheck(HWND windowHandle, int controlIdentifier);
void ccwButtonUncheck(HWND windowHandle, int controlIdentifier);
void ccwButtonCheckConditional(HWND windowHandle, int controlIdentifier, BOOLE check);
void ccwButtonDisable(HWND windowHandle, int controlIdentifier);
void ccwButtonEnable(HWND windowHandle, int controlIdentifier);
BOOLE ccwButtonGetCheck(HWND windowHandle, int controlIdentifier);
void ccwSliderSetRange(HWND windowHandle, int controlIdentifier, ULO minPos, ULO maxPos);
ULO ccwComboBoxGetCurrentSelection(HWND windowHandle, int controlIdentifier);
void ccwComboBoxSetCurrentSelection(HWND windowHandle, int controlIdentifier, ULO index);
void ccwComboBoxAddString(HWND windowHandle, int controlIdentifier, STR *text);
ULO ccwSliderGetPosition(HWND windowHandle, int controlIdentifier);
void ccwSliderSetPosition(HWND windowHandle, int controlIdentifier, LONG position);
void ccwEditSetText(HWND windowHandle, int controlIdentifier, STR *text);
void ccwEditGetText(HWND windowHandle, int controlIdentifier, STR *text, ULO n);
void ccwEditEnableConditional(HWND windowHandle, int controlIdentifier, BOOLE enable);



#endif

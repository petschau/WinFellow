#ifndef COMMONCONTROLWRAP_H
#define COMMONCONTROLWRAP_H

extern void ccwButtonSetCheck(HWND windowHandle, int controlIdentifier);
extern void ccwButtonUncheck(HWND windowHandle, int controlIdentifier);
extern void ccwButtonCheckConditional(HWND windowHandle, int controlIdentifier, BOOLE check);
extern void ccwButtonDisable(HWND windowHandle, int controlIdentifier);
extern void ccwButtonEnable(HWND windowHandle, int controlIdentifier);
extern BOOLE ccwButtonGetCheck(HWND windowHandle, int controlIdentifier);
extern bool ccwButtonGetCheckBool(HWND windowHandle, int controlIdentifier);
extern void ccwButtonEnableConditional(HWND windowHandle, int controlIdentifier, BOOLE enable);
extern void ccwSliderSetRange(HWND windowHandle, int controlIdentifier, ULO minPos, ULO maxPos);
extern ULO ccwComboBoxGetCurrentSelection(HWND windowHandle, int controlIdentifier);
extern void ccwComboBoxSetCurrentSelection(HWND windowHandle, int controlIdentifier, ULO index);
extern void ccwComboBoxAddString(HWND windowHandle, int controlIdentifier, STR *text);
extern ULO ccwSliderGetPosition(HWND windowHandle, int controlIdentifier);
extern void ccwSliderSetPosition(HWND windowHandle, int controlIdentifier, LONG position);
extern void ccwSliderEnable(HWND windowHandle, int controlIdentifier, BOOL enable);
extern void ccwStaticSetText(HWND windowHandle, int controlIdentifier, STR *text);
extern void ccwEditSetText(HWND windowHandle, int controlIdentifier, STR *text);
extern void ccwEditGetText(HWND windowHandle, int controlIdentifier, STR *text, ULO n);
extern void ccwEditEnableConditional(HWND windowHandle, int controlIdentifier, BOOLE enable);
extern void ccwEditEnableConditionalBool(HWND windowHandle, int controlIdentifier, bool enable);
extern void ccwSetImageConditional(HWND windowHandle, int controlIdentifier, HBITMAP bitmapFirst, HBITMAP bitmapSecond, BOOLE setFirst);
extern void ccwMenuCheckedSetConditional(HWND windowHandle, int menuIdentifier, BOOLE enable);
extern BOOLE ccwMenuCheckedToggle(HWND windowHandle, int menuIdentifier);

#endif

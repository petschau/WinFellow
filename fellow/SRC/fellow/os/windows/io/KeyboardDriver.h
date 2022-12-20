#pragma once

#include "fellow/api/drivers/IKeyboardDriver.h"
#include "fellow/os/windows/dxver.h"

class KeyboardDriver : public IKeyboardDriver
{
private:
  BOOLE _active;
  LPDIRECTINPUT _lpDI;
  LPDIRECTINPUTDEVICE _lpDID;
  HANDLE _DIevent;
  BYTE _keys[MAX_KEYS];     // contains boolean values (pressed/not pressed) for actual keystroke
  BYTE _prevkeys[MAX_KEYS]; // contains boolean values (pressed/not pressed) for past keystroke
  bool _initialization_failed;
  BOOLE _prs_rewrite_mapping_file;
  char _mapping_filename[MAX_PATH];
  bool _kbd_in_task_switcher = false;

  void ClearPressedKeys();
  const char *DInputErrorString(HRESULT hResult);
  const char *DInputUnaquireReturnValueString(HRESULT hResult);
  void DInputFailure(const char *header, HRESULT err);
  void DInputUnacquireFailure(const char *header, HRESULT err);
  bool DInputSetCooperativeLevel();
  void DInputAcquireFailure(const char *header, HRESULT err);
  void DInputUnacquire();
  void DInputAcquire();
  void DInputRelease();
  bool DInputInitialize();
  BOOLE EventChecker(kbd_drv_pc_symbol symbol_key);
  void Keypress(ULO keycode, BOOL pressed);
  void BufferOverflowHandler();
  const char *DikKeyString(int dikkey);
  void InitializeDIKToSymbolKeyTable();

public:
  const char *KeyString(kbd_drv_pc_symbol symbolickey) override;
  const char *KeyPrettyString(kbd_drv_pc_symbol symbolickey) override;
  void JoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey) override;
  kbd_drv_pc_symbol JoystickReplacementGet(kbd_event event) override;
  void KeypressRaw(ULO lRawKeyCode, BOOLE pressed) override;
  void StateHasChanged(BOOLE active) override;
  void KeypressHandler() override;

#ifdef RETRO_PLATFORM
  void EOFHandler() override;
  void SetJoyKeyEnabled(ULO, ULO, BOOLE) override;
#endif

  void HardReset() override;
  void EmulationStart() override;
  void EmulationStop() override;
  void Startup() override;
  void Shutdown() override;
};

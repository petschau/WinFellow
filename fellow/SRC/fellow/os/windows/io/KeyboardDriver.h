#pragma once

#include "fellow/api/drivers/IKeyboardDriver.h"
#include "fellow/os/windows/dxver.h"

constexpr auto PCSymbolCount = 106;
constexpr auto DIKSymbolCount = 256;

class KeyboardDriver : public IKeyboardDriver
{
private:
  Keyboard *_keyboard{};

  bool _active = false;
  LPDIRECTINPUT _lpDI = nullptr;
  LPDIRECTINPUTDEVICE _lpDID = nullptr;
  HANDLE _di_event = nullptr;
  BYTE _keys[DIKSymbolCount]{};     // contains boolean values (pressed/not pressed) for actual keystroke
  BYTE _prevkeys[DIKSymbolCount]{}; // contains boolean values (pressed/not pressed) for past keystroke
  bool _prs_rewrite_mapping_file = false;
  char _mapping_filename[MAX_PATH]{};
  bool _initializationFailed = false;
  bool _kbd_in_task_switcher = false;

  kbd_drv_pc_symbol _joykey[2][MAX_JOYKEY_VALUE]{};  // Keys for joykeys 0 and 1
  bool _joykey_enabled[2][2]{};                      // For each port, the enabled joykeys
  kbd_event _joykey_event[2][2][MAX_JOYKEY_VALUE]{}; // Event ID for each joykey [port][pressed][direction]
  volatile kbd_drv_pc_symbol _captured_key = PCK_NONE;
  bool _capture = false;

  kbd_drv_pc_symbol _dik_to_symbol[DIKSymbolCount]{};

  static const char *_pc_symbol_to_string[PCSymbolCount];
  static const char *_symbol_pretty_name[PCSymbolCount];
  static int _symbol_to_dik[PCSymbolCount];
  static UBY _pc_symbol_to_amiga_scancode[PCSymbolCount];

  kbd_drv_pc_symbol ToSymbolicKey(unsigned int scancode) const;
  int Map(kbd_drv_pc_symbol sym) const;
  bool WasPressed(kbd_drv_pc_symbol sym) const;
  bool IsPressed(kbd_drv_pc_symbol sym) const;
  bool Released(kbd_drv_pc_symbol sym) const;
  bool Pressed(kbd_drv_pc_symbol sym) const;

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
  bool EventChecker(kbd_drv_pc_symbol symbol_key);
  void Keypress(ULO keycode, BOOL pressed);
  void BufferOverflowHandler();
  const char *GetDIKDescription(int dikkey);
  void InitializeDIKToSymbolKeyTable();

public:
  HANDLE GetDirectInputEvent() const;

  kbd_drv_pc_symbol GetPCSymbolFromDescription(const char *pcSymbolDescription) override;
  const char *GetPCSymbolDescription(kbd_drv_pc_symbol symbolickey) override;
  const char *GetPCSymbolPrettyDescription(kbd_drv_pc_symbol symbolickey) override;
  kbd_drv_pc_symbol GetPCSymbolFromDIK(int dikkey) override;

  void JoystickReplacementSet(kbd_event event, kbd_drv_pc_symbol symbolickey) override;
  kbd_drv_pc_symbol JoystickReplacementGet(kbd_event event) override;
  void KeypressRaw(ULO lRawKeyCode, bool pressed) override;
  void StateHasChanged(bool active) override;
  void KeypressHandler() override;

#ifdef RETRO_PLATFORM
  void EOFHandler() override;
  void SetJoyKeyEnabled(ULO lGameport, ULO lSetting, bool bEnabled) override;
#endif

  void HardReset() override;
  void EmulationStart() override;
  void EmulationStop() override;

  bool GetInitializationFailed() override;
  void Initialize(Keyboard *keyboard) override;
  void Release() override;

  KeyboardDriver();
  virtual ~KeyboardDriver();
};

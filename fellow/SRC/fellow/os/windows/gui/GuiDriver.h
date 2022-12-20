#pragma once

#include <CommCtrl.h>
#include <functional>

#include "fellow/api/drivers/IGuiDriver.h"
#include "fellow/api/drivers/IKeyboardDriver.h"
#include "fellow/os/windows/gui/GuiDrawmode.h"
#include "fellow/os/windows/gui/GuiPresets.h"
#include "fellow/configuration/Configuration.h"
#include "fellow/api/drivers/IniValues.h"
#include "fellow/chipset/Kbd.h"

enum class SelectFileFlags
{
  FSEL_ROM = 0,
  FSEL_ADF = 1,
  FSEL_KEY = 2,
  FSEL_HDF = 3,
  FSEL_WFC = 4,
  FSEL_MOD = 5,
  FSEL_FST = 6
};

//================================
// Flags for various global events
//================================

enum class wguiActions
{
  WGUI_NO_ACTION,
  WGUI_START_EMULATION,
  WGUI_QUIT_EMULATOR,
  WGUI_CONFIGURATION,
  WGUI_OPEN_CONFIGURATION,
  WGUI_SAVE_CONFIGURATION,
  WGUI_SAVE_CONFIGURATION_AS,
  WGUI_LOAD_HISTORY0,
  WGUI_LOAD_HISTORY1,
  WGUI_LOAD_HISTORY2,
  WGUI_LOAD_HISTORY3,
  WGUI_DEBUGGER_START,
  WGUI_ABOUT,
  WGUI_LOAD_STATE,
  WGUI_SAVE_STATE,
  WGUI_PAUSE_EMULATION_WHEN_WINDOW_LOSES_FOCUS,
  WGUI_GFX_DEBUG_IMMEDIATE_RENDERING,
  WGUI_RIPMODULES,
  WGUI_DUMP_MEMORY
};

constexpr unsigned int MAX_JOYKEY_PORT = 2;
constexpr unsigned int NUMBER_OF_GAMEPORT_STRINGS = 6;
constexpr unsigned int MAX_DISKDRIVES = 4;
constexpr unsigned int DISKDRIVE_PROPERTIES = 3;
constexpr unsigned int DISKDRIVE_PROPERTIES_MAIN = 4;
constexpr unsigned int NUMBER_OF_CHIPRAM_SIZES = 8;
constexpr unsigned int NUMBER_OF_FASTRAM_SIZES = 5;
constexpr unsigned int NUMBER_OF_BOGORAM_SIZES = 8;
constexpr unsigned int NUMBER_OF_SOUND_RATES = 4;
constexpr unsigned int NUMBER_OF_SOUND_FILTERS = 3;
constexpr unsigned int NUMBER_OF_CPUS = 10;
constexpr unsigned int PROP_SHEETS = 10;

class GuiDriver : public IGuiDriver
{
private:
  HWND _hMainDialog;
  STR _extractedfilename[CFG_FILENAME_LENGTH];
  STR _extractedpathname[CFG_FILENAME_LENGTH];
  cfg *_wgui_cfg;       // GUI copy of configuration
  IniValues *_wgui_ini; // GUI copy of initdata
  wgui_drawmodes _wgui_dm; // data structure for resolution data
  wgui_drawmode *_pwgui_dm_match;
  bool _wgui_emulation_state = false;
  HBITMAP _power_led_on_bitmap = nullptr;
  HBITMAP _power_led_off_bitmap = nullptr;
  HBITMAP _diskdrive_led_disabled_bitmap = nullptr;
  HBITMAP _diskdrive_led_off_bitmap = nullptr;

  static const kbd_event _gameport_keys_events[MAX_JOYKEY_PORT][MAX_JOYKEY_VALUE];
  static const int _gameport_keys_labels[MAX_JOYKEY_PORT][MAX_JOYKEY_VALUE];
  static const char *_wgui_gameport_strings[NUMBER_OF_GAMEPORT_STRINGS];
  static const int _diskimage_data[MAX_DISKDRIVES][DISKDRIVE_PROPERTIES];
  static const int _diskimage_data_main[MAX_DISKDRIVES][DISKDRIVE_PROPERTIES_MAIN];
  static const char *_chipram_strings[NUMBER_OF_CHIPRAM_SIZES];
  static const char *_fastram_strings[NUMBER_OF_FASTRAM_SIZES];
  static const char *_bogoram_strings[NUMBER_OF_BOGORAM_SIZES];
  static const int _sound_rates_cci[NUMBER_OF_SOUND_RATES];
  static const int _sound_filters_cci[NUMBER_OF_SOUND_FILTERS];
  static const int _cpus_cci[NUMBER_OF_CPUS];

  // preset handling
  static STR _preset_path[CFG_FILENAME_LENGTH];
  ULO _num_presets = 0;
  wgui_preset *_presets = nullptr;

  wguiActions _wgui_action;

  //====================================================================
  // The following tables defines the data needed to create the property
  // sheets for the configuration dialog box.
  // -Number of property sheets
  // -Resource ID for each property sheet
  // -Dialog procedure for each property sheet
  //====================================================================

  static const UINT _propsheetRID[PROP_SHEETS];
  static const UINT _propsheetICON[PROP_SHEETS];

  // in this struct, we remember the configuration dialog property sheet handles,
  // so that a refresh can be triggered by the presets propery sheet
  HWND _propsheetHWND[PROP_SHEETS] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

  typedef INT_PTR (*wguiDlgProc)(HWND, UINT, WPARAM, LPARAM);

  wguiDlgProc _propsheetDialogProc[PROP_SHEETS];

  template <INT_PTR (GuiDriver::*P)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)>
  wguiDlgProc DialogCallbackWrapper()
  {
    return [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> INT_PTR
    {
      if (uMsg == WM_INITDIALOG)
      {
        SetWindowLongPtr(hWnd, DWLP_USER, lParam);
      }

      GuiDriver *pThis = reinterpret_cast<GuiDriver *>(GetWindowLongPtr(hWnd, DWLP_USER));
      return pThis ? (pThis->*P)(hWnd, uMsg, wParam, lParam) : FALSE;
    };
  };

  // Creates a dialog box and returns, does not wait for it to finish
  // Redirects the dialog's window procedure to the current instance object of the UI (this)
  template <INT_PTR (GuiDriver::*P)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)>
  HWND CreateDialogBox(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent)
  {
    return ::CreateDialogParam(hInstance, lpTemplateName, hWndParent, DialogCallbackWrapper<P>(),reinterpret_cast<LPARAM>(this));
  };

  // Creates a dialog box and waits for it to finish, typically OK or Cancel type dialog
  // Redirects the dialogs's window procedure to the current instance object of the UI (this)
  template <INT_PTR (GuiDriver::*P)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)>
  INT_PTR ExecuteDialogBox(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent)
  {
    return ::DialogBoxParam(hInstance, lpTemplateName, hWndParent, DialogCallbackWrapper<P>(), reinterpret_cast<LPARAM>(this));
  };

  int ShowMessageForEmulator(const char *szMessage, UINT uType = MB_OK);
  int ShowMessageForMainDialog(const char *message, const char *title, UINT type = MB_OK);
  int ShowMessage(const char *message, const char *title, HWND sourceDialog, UINT type = MB_OK);
  UINT GetRequesterTypeFromFellowRequesterType(FELLOW_REQUESTER_TYPE fellowRequesterType);

  static bool InitializePresets(wgui_preset **wgui_presets, ULO *wgui_num_presets);
  bool CheckEmulationNecessities();

  public:
  INT_PTR VariousDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  INT_PTR ConfigurationDialog();
  void SetCheckOfUseMultipleGraphicalBuffers(BOOLE useMultipleGraphicalBuffers);

  INT_PTR MainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void About(HWND hwndDlg);
  INT_PTR AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  INT_PTR GameportDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  INT_PTR HardfileDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  INT_PTR HardfileAddDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void HardfileAddDialogSetGeometryEdits(
      HWND hwndDlg, STR *filename, bool readonly, int sectorsPerTrack, int surfaces, int reservedBlocks, int bytesPerSector, bool enable);
  void HardfileAddDialogEnableGeometry(HWND hwndDlg, bool enable);
  INT_PTR HardfileCreateDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  INT_PTR FilesystemDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


  void LoadBitmaps();
  void ReleaseBitmaps();
  void CheckMemorySettingsForChipset();
  wgui_drawmode_list &GetFullScreenMatchingList(ULO colorbits);
  int GetDesktopBitsPerPixel();
  std::pair<unsigned int, unsigned int> GetDesktopSize();
  std::pair<unsigned int, unsigned int> GetDesktopWorkAreaSize();
  wgui_drawmode *GetUIDrawModeFromIndex(unsigned int index, wgui_drawmode_list &list);
  void GetResolutionStrWithIndex(LONG index, char char_buffer[]);
  void GetFrameSkippingStrWithIndex(LONG index, char char_buffer[]);
  void SetSliderTextAccordingToPosition(
      HWND windowHandle, int sliderIdentifier, int sliderTextIdentifier, std::function<void(LONG, char[])> getSliderStrWithIndex);
  ULO GetColorBitsFromComboboxIndex(LONG index);
  LONG GetComboboxIndexFromColorBits(ULO colorbits);
  DISPLAYDRIVER GetDisplayDriverFromComboboxIndex(LONG index);
  LONG GetComboboxIndexFromDisplayDriver(DISPLAYDRIVER displaydriver);
  void ConvertDrawModeListToGuiDrawModes();
  void FreeGuiDrawModesLists();
  wgui_drawmode *MatchFullScreenResolution();
  STR *ExtractFilename(STR *fullpathname);
  const char *GetBOOLEToString(BOOLE value);
  BOOLE SelectFile(HWND hwndDlg, STR *filename, ULO filenamesize, const char *title, SelectFileFlags SelectFileType);
  BOOLE SelectDirectory(HWND hwndDlg, STR *szPath, STR *szDescription, ULO filenamesize, const char *szTitle);
  void RemoveAllHistory();
  void InstallHistoryIntoMenu();
  void PutCfgInHistoryOnTop(ULO cfgtotop);
  void InsertCfgIntoHistory(const std::string& cfgfilenametoinsert);
  void DeleteCfgFromHistory(ULO itemtodelete);
  void SwapCfgsInHistory(ULO itemA, ULO itemB);
  bool SaveConfigurationFileAs(cfg *conf, HWND hwndDlg);
  void OpenConfigurationFile(cfg *conf, HWND hwndDlg);
  void InstallCPUConfig(HWND hwndDlg, cfg *conf);
  void ExtractCPUConfig(HWND hwndDlg, cfg *conf);
  void InstallFloppyConfig(HWND hwndDlg, cfg *conf);
  void InstallFloppyMain(HWND hwndDlg, cfg *conf);
  void ExtractFloppyConfig(HWND hwndDlg, cfg *conf);
  void ExtractFloppyMain(HWND hwndDlg, cfg *conf);
  void InstallMemoryConfig(HWND hwndDlg, cfg *conf);
  void ExtractMemoryConfig(HWND hwndDlg, cfg *conf);
  void InstallBlitterConfig(HWND hwndDlg, cfg *conf);
  void ExtractBlitterConfig(HWND hwndDlg, cfg *conf);
  void InstallSoundConfig(HWND hwndDlg, cfg *conf);
  void ExtractSoundConfig(HWND hwndDlg, cfg *conf);
  void InstallGameportConfig(HWND hwndDlg, cfg *conf);
  void ExtractGameportConfig(HWND hwndDlg, cfg *conf);
  void InstallVariousConfig(HWND hwndDlg, cfg *conf);
  void ExtractVariousConfig(HWND hwndDlg, cfg *conf);
  void InstallMenuPauseEmulationWhenWindowLosesFocus(HWND hwndDlg, IniValues *ini);
  void ToggleMenuPauseEmulationWhenWindowLosesFocus(HWND hwndDlg, IniValues *ini);
  void InstallMenuGfxDebugImmediateRendering(HWND hwndDlg, IniValues *ini);
  void ToggleMenuGfxDebugImmediateRendering(HWND hwndDlg, IniValues *ini);
  void HardfileSetInformationString(STR *s, const char *deviceName, int partitionNumber, const HardfilePartition &partition);
  HTREEITEM HardfileTreeViewAddDisk(HWND hwndTree, STR *filename, rdb_status rdbStatus, const HardfileGeometry &geometry, int hardfileIndex);
  void HardfileTreeViewAddPartition(
      HWND hwndTree, HTREEITEM parent, int partitionNumber, const char *deviceName, const HardfilePartition &partition, int hardfileIndex);
  void HardfileTreeViewAddHardfile(HWND hwndTree, cfg_hardfile *hf, int hardfileIndex);
  void InstallHardfileConfig(HWND hwndDlg, cfg *conf);
  void ExtractHardfileConfig(HWND hwndDlg, cfg *conf);
  bool HardfileAdd(HWND hwndDlg, cfg *conf, bool add, ULO index, cfg_hardfile *target);
  bool HardfileCreate(HWND hwndDlg, cfg *conf, ULO index, cfg_hardfile *target);
  void FilesystemUpdate(HWND lvHWND, cfg_filesys *fs, ULO i, BOOL add, STR *prefix);
  void InstallFilesystemConfig(HWND hwndDlg, cfg *conf);
  void ExtractFilesystemConfig(HWND hwndDlg, cfg *conf);
  BOOLE FilesystemAdd(HWND hwndDlg, cfg *conf, BOOLE add, ULO index, cfg_filesys *target);
  void InstallDisplayScaleConfigInGUI(HWND hwndDlg, cfg *conf);
  void ExtractDisplayScaleConfigFromGUI(HWND hwndDlg, cfg *conf);
  void InstallColorBitsConfigInGUI(HWND hwndDlg, cfg *conf);
  void InstallFullScreenButtonConfigInGUI(HWND hwndDlg, cfg *conf);
  void InstallDisplayScaleStrategyConfigInGUI(HWND hwndDlg, cfg *conf);
  void InstallEmulationAccuracyConfigInGUI(HWND hwndDlg, cfg *conf);
  void ExtractEmulationAccuracyConfig(HWND hwndDlg, cfg *conf);
  void InstallFullScreenResolutionConfigInGUI(HWND hwndDlg, cfg *conf);
  void InstallDisplayDriverConfigInGUI(HWND hwndDlg, cfg *conf);
  void InstallFrameSkipConfigInGUI(HWND hwndDlg, cfg *conf);
  void InstallDisplayConfig(HWND hwndDlg, cfg *conf);
  unsigned int DecideScaleFromDesktop(unsigned int unscaled_width, unsigned int unscaled_height);
  void ExtractDisplayFullscreenConfig(HWND hwndDlg, cfg *cfg);
  void ExtractDisplayConfig(HWND hwndDlg, cfg *conf);
  LON ListViewNext(HWND ListHWND, ULO initialindex);
  LPARAM TreeViewSelection(HWND hwndTree);
  INT_PTR CPUDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  INT_PTR PresetDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void SelectDiskImage(cfg *conf, HWND hwndDlg, int editIdentifier, ULO index);
  bool CreateFloppyDiskImage(cfg *conf, HWND hwndDlg, ULO index);
  INT_PTR FloppyDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  INT_PTR FloppyCreateDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  INT_PTR MemoryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  ULO GetNumberOfScreenAreas(ULO colorbits);
  INT_PTR DisplayDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL BlitterDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  INT_PTR SoundDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  INT_PTR FilesystemAddDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
  BOOLE SaveFile(HWND hwndDlg, char *filename, ULO filenamesize, const char *title, SelectFileFlags SelectFileType);
  STR *ExtractPath(STR *fullpathname);
  void Requester(FELLOW_REQUESTER_TYPE type, const char *szMessage) override;
  void SetProcessDPIAwareness(const char *pszAwareness) override;
  BOOLE Enter() override;
  void Startup() override;
  void StartupPost();
  void Shutdown() override;
};

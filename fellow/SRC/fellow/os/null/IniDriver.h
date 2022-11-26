#pragma once

#include "fellow/api/defs.h"
#include "fellow/api/drivers/IIniDriver.h"

class IniDriver : public IIniDriver
{
private:
  const char *INI_FILENAME = "WinFellow.ini";
  IniValues *m_current_ini;
  IniValues *m_default_ini;

  BOOLE SetOption(IniValues *initdata, STR *initoptionstr);
  ULO GetULOFromString(STR *value);
  int GetintFromString(STR *value);
  BOOLE GetBOOLEFromString(STR *value);
  void StripTrailingNewlines(STR *line);

  BOOLE LoadIniFile(IniValues *initdata, FILE *inifile);
  BOOLE LoadIniFromFilename(IniValues *inidata, STR *filename);

  BOOLE SaveOptions(IniValues *initdata, FILE *inifile);
  BOOLE SaveToFilename(IniValues *initdata, STR *filename);
  BOOLE SaveToFile(IniValues *initdata, FILE *inifile);

  IniValues *GetNewIni();
  void FreeIni(IniValues *initdata);

public:
  void SetCurrentInitdata(IniValues *initdata);
  IniValues *GetCurrentInitdata();

  void SetDefaultInitdata(IniValues *initdata);
  IniValues *GetDefaultInitdata();

  void EmulationStart() override;
  void EmulationStop() override;
  void Startup() override;
  void Shutdown() override;
};

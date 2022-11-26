#pragma once

#include <string>
#include "fellow/api/defs.h"
#include "fellow/api/drivers/IIniDriver.h"

class IniDriver : public IIniDriver
{
private:
  const char *INI_FILENAME = "WinFellow.ini";
  std::string _defaultIniFilePath;

  IniValues *m_current_ini;
  IniValues *m_default_ini;

  BOOLE SetOption(IniValues *initdata, const std::string &initoptionstr);
  ULO GetULOFromString(const std::string &value);
  int GetintFromString(const std::string &value);
  BOOLE GetBOOLEFromString(const std::string &value);
  void StripTrailingNewlines(STR *line);

  BOOLE LoadIniFile(IniValues *initdata, FILE *inifile);
  BOOLE LoadIniFromFilename(IniValues *inidata, STR *filename);

  BOOLE SaveOptions(IniValues *initdata, FILE *inifile);
  BOOLE SaveToFilename(IniValues *initdata, const std::string &filename);
  BOOLE SaveToFile(IniValues *initdata, FILE *inifile);

  IniValues *GetNewIni();
  void FreeIni(IniValues *initdata);

public:
  void SetCurrentInitdata(IniValues *initdata) override;
  IniValues *GetCurrentInitdata() override;

  void SetDefaultInitdata(IniValues *initdata) override;
  IniValues *GetDefaultInitdata() override;

  void EmulationStart() override;
  void EmulationStop() override;
  void Startup() override;
  void Shutdown() override;
};

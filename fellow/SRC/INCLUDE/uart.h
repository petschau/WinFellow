#ifndef UART_H
#define UART_H

#include "DEFS.H"
#include <string>

class UART
{
private:
  std::string _outputFileName;
  FILE *_outputFile;

  UWO _serper;
  UWO _serdat;

  UWO _transmitBuffer;
  UWO _transmitShiftRegister;
  ULO _transmitDoneTime;
  bool _transmitBufferEmpty;
  bool _transmitShiftRegisterEmpty;

  UWO _receiveBuffer;
  UWO _receiveShiftRegister;
  ULO _receiveDoneTime;
  bool _receiveBufferFull;
  bool _receiveBufferOverrun;

  void InstallIOHandlers();

  void ClearState();
  void LoadState(FILE *F);
  void SaveState(FILE *F);  

  void OpenOutputFile();
  void CloseOutputFile();

  bool Is8BitMode();
  UWO GetBitPeriod();

  void CopyReceiveShiftRegisterToBuffer();
  void CopyTransmitBufferToShiftRegister();
  ULO GetTransmitDoneTime();

public:
  static void wserper(UWO data, ULO address);
  static void wserdat(UWO data, ULO address);
  static UWO rserdat(ULO address);

  UWO ReadSerdatRegister();
  void WriteSerdatRegister(UWO data);
  void WriteSerperRegister(UWO data);

  void NotifyInterruptRequestBitsChanged(UWO intreq);

  void EndOfLine();
  void EndOfFrame();

  void EmulationStart();
  void EmulationStop();

  UART();
  ~UART();
};

extern UART uart;

#endif

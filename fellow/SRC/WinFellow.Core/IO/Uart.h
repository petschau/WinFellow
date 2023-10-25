#pragma once

#include <string>

class Uart
{
private:
  std::string _outputFileName;
  FILE *_outputFile;

  uint16_t _serper;

  uint16_t _transmitBuffer;
  uint16_t _transmitShiftRegister;
  uint32_t _transmitDoneTime;
  bool _transmitBufferEmpty;
  bool _transmitShiftRegisterEmpty;

  uint16_t _receiveBuffer;
  uint16_t _receiveShiftRegister;
  uint32_t _receiveDoneTime;
  bool _receiveBufferFull;
  bool _receiveBufferOverrun;

  void InstallIOHandlers();

  void ClearState();
  void LoadState(FILE *F);
  void SaveState(FILE *F);

  void OpenOutputFile();
  void CloseOutputFile();

  bool Is8BitMode();
  uint16_t GetBitPeriod();

  void CopyReceiveShiftRegisterToBuffer();
  void CopyTransmitBufferToShiftRegister();
  uint32_t GetTransmitDoneTime();

public:
  static void wserper(uint16_t data, uint32_t address);
  static void wserdat(uint16_t data, uint32_t address);
  static uint16_t rserdat(uint32_t address);

  uint16_t ReadSerdatRegister();
  void WriteSerdatRegister(uint16_t data);
  void WriteSerperRegister(uint16_t data);

  void NotifyInterruptRequestBitsChanged(uint16_t intreq);

  void EndOfLine();
  void EndOfFrame();

  void EmulationStart();
  void EmulationStop();

  Uart();
  ~Uart();
};

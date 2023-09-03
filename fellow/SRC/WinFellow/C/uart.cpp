#include "uart.h"
#include "BUS.H"
#include "FMEM.H"
#include "interrupt.h"
#include "fileops.h"

UART uart;

/* SERDATR 0xdff018 */
UWO UART::rserdat(ULO address)
{
  return uart.ReadSerdatRegister();
}

/* SERDAT 0xdff030 */
void UART::wserdat(UWO data, ULO address)
{
  uart.WriteSerdatRegister(data);
}

/* SERDAT 0xdff032 */
void UART::wserper(UWO data, ULO address)
{
  uart.WriteSerperRegister(data);
}

UWO UART::ReadSerdatRegister()
{
  UWO data = _receiveBuffer & 0x3ff;

  // Bit 11 is RXD live

  if (_transmitShiftRegisterEmpty) data |= 0x1000;
  if (_transmitBufferEmpty) data |= 0x2000;
  if (_receiveBufferFull) data |= 0x4000;
  if (_receiveBufferOverrun) data |= 0x8000;

  return data;
}

void UART::WriteSerdatRegister(UWO data)
{
  _transmitBuffer = data & 0x3ff;
  _transmitBufferEmpty = false;

  if (_transmitBuffer != 0)
  {
    CopyTransmitBufferToShiftRegister();
  }
}

void UART::WriteSerperRegister(UWO data)
{
  _serper = data;  
}

ULO UART::GetTransmitDoneTime()
{
  int bitsToTransfer = 2 + (Is8BitMode() ? 8 : 9);
  ULO cyclesPerBit = GetBitPeriod() + 1;

  return cyclesPerBit * bitsToTransfer;  
}

void UART::CopyTransmitBufferToShiftRegister()
{
  if (_transmitShiftRegisterEmpty)
  {
    _transmitShiftRegister = _transmitBuffer;
    _transmitShiftRegisterEmpty = false;
    _transmitBufferEmpty = true;
    _transmitDoneTime = GetTransmitDoneTime() + busGetCycle();

    wintreq_direct(0x8001, 0xdff09c, true); // TBE interrupt

#ifdef _DEBUG
    OpenOutputFile();
    fwrite(&_transmitBuffer, 1, 1, _outputFile);
#endif
  }
}

void UART::CopyReceiveShiftRegisterToBuffer()
{
  _receiveBuffer = _receiveShiftRegister;
  _receiveBufferFull = true;  
  wintreq_direct(0x8400, 0xdff09c, true); // RBF interrupt
}

void UART::NotifyInterruptRequestBitsChanged(UWO intreq)
{
  // Clear only, or is it directly wired?
  // HRM says overrun is also mirrored from intreq? How?
  _receiveBufferFull = (intreq & 0x0800) == 0x0800;  
  if (!_receiveBufferFull)
  {
    _receiveBufferOverrun = false;
  }
}

void UART::InstallIOHandlers()
{
  memorySetIoWriteStub(0x030, wserdat);
  memorySetIoWriteStub(0x32, wserper);
  memorySetIoReadStub(0x018, rserdat);
}

void UART::ClearState()
{
  _serper = 0;

  _transmitBuffer = 0;
  _transmitShiftRegister = 0;
  _transmitBufferEmpty = true;
  _transmitShiftRegisterEmpty = true;
  _transmitDoneTime = BUS_CYCLE_DISABLE;

  _receiveBuffer = 0;
  _receiveShiftRegister = 0;
  _receiveBufferFull = false;
  _receiveBufferOverrun = false;
  _receiveDoneTime = BUS_CYCLE_DISABLE;
}

void UART::LoadState(FILE *F)
{
  fread(&_serper, sizeof(_serper), 1, F);
  fread(&_transmitBuffer, sizeof(_transmitBuffer), 1, F);
  fread(&_transmitShiftRegister, sizeof(_transmitShiftRegister), 1, F);
  fread(&_transmitDoneTime, sizeof(_transmitDoneTime), 1, F);
  fread(&_transmitBufferEmpty, sizeof(_transmitBufferEmpty), 1, F);
  fread(&_transmitShiftRegisterEmpty, sizeof(_transmitShiftRegisterEmpty), 1, F);
  fread(&_receiveBuffer, sizeof(_receiveBuffer), 1, F);
  fread(&_receiveShiftRegister, sizeof(_receiveShiftRegister), 1, F);
  fread(&_receiveDoneTime, sizeof(_receiveDoneTime), 1, F);
  fread(&_receiveBufferFull, sizeof(_receiveBufferFull), 1, F);
  fread(&_receiveBufferOverrun, sizeof(_receiveBufferOverrun), 1, F);
}

void UART::SaveState(FILE *F)
{
  fwrite(&_serper, sizeof(_serper), 1, F);
  fwrite(&_transmitBuffer, sizeof(_transmitBuffer), 1, F);
  fwrite(&_transmitShiftRegister, sizeof(_transmitShiftRegister), 1, F);
  fwrite(&_transmitDoneTime, sizeof(_transmitDoneTime), 1, F);
  fwrite(&_transmitBufferEmpty, sizeof(_transmitBufferEmpty), 1, F);
  fwrite(&_transmitShiftRegisterEmpty, sizeof(_transmitShiftRegisterEmpty), 1, F);
  fwrite(&_receiveBuffer, sizeof(_receiveBuffer), 1, F);
  fwrite(&_receiveShiftRegister, sizeof(_receiveShiftRegister), 1, F);
  fwrite(&_receiveDoneTime, sizeof(_receiveDoneTime), 1, F);
  fwrite(&_receiveBufferFull, sizeof(_receiveBufferFull), 1, F);
  fwrite(&_receiveBufferOverrun, sizeof(_receiveBufferOverrun), 1, F);
}

bool UART::Is8BitMode()
{
  return (_serper & 0x8000) == 0;
}

UWO UART::GetBitPeriod()
{
  return _serper & 0x3fff;
}

void UART::OpenOutputFile()
{
  if (_outputFile == nullptr)
  {
    _outputFile = fopen(_outputFileName.c_str(), "wb");
  }
}

void UART::CloseOutputFile()
{
  if (_outputFile != nullptr)
  {
    fclose(_outputFile);
    _outputFile = nullptr;
  }  
}

void UART::EndOfLine()
{
  // (Put this in an event with an exact time-stamp)

  if (_transmitDoneTime <= busGetCycle())
  {
    _transmitShiftRegisterEmpty = true;
    _transmitDoneTime = BUS_CYCLE_DISABLE;

    if (!_transmitBufferEmpty)
    {
      CopyTransmitBufferToShiftRegister();
    }
  }

  if (_receiveDoneTime <= busGetCycle())
  {
    _receiveDoneTime = BUS_CYCLE_DISABLE;

    if (!_receiveBufferFull)
    {
      CopyReceiveShiftRegisterToBuffer();
    }
    else
    {
      _receiveBufferOverrun = true;
    }
  }
}

void UART::EndOfFrame()
{
  if (_transmitDoneTime != BUS_CYCLE_DISABLE)
  {
    _transmitDoneTime -= busGetCyclesInThisFrame();
    if ((int)_transmitDoneTime < 0)
    {
      _transmitDoneTime = 0;
    }
  }  
  if (_receiveDoneTime != BUS_CYCLE_DISABLE)
  {
    _receiveDoneTime -= busGetCyclesInThisFrame();
    if ((int)_receiveDoneTime < 0)
    {
      _receiveDoneTime = 0;
    }
  }
}

void UART::EmulationStart()
{
  InstallIOHandlers();
}

void UART::EmulationStop()
{
  CloseOutputFile();
}

UART::UART()
  : _outputFile(nullptr)
{
  char tempFileName[256];
  fileopsGetGenericFileName(tempFileName, "WinFellow", "uart_output.bin");
  _outputFileName = tempFileName;
  ClearState();
}

UART::~UART()
{
  CloseOutputFile();
}

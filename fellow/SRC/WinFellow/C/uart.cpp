#include "IO/Uart.h"
#include "BUS.H"
#include "FMEM.H"
#include "interrupt.h"
#include "VirtualHost/Core.h"

/* SERDATR 0xdff018 */
uint16_t Uart::rserdat(uint32_t address)
{
  return _core.Uart->ReadSerdatRegister();
}

/* SERDAT 0xdff030 */
void Uart::wserdat(uint16_t data, uint32_t address)
{
  _core.Uart->WriteSerdatRegister(data);
}

/* SERDAT 0xdff032 */
void Uart::wserper(uint16_t data, uint32_t address)
{
  _core.Uart->WriteSerperRegister(data);
}

uint16_t Uart::ReadSerdatRegister()
{
  uint16_t data = _receiveBuffer & 0x3ff;

  // Bit 11 is RXD live

  if (_transmitShiftRegisterEmpty) data |= 0x1000;
  if (_transmitBufferEmpty) data |= 0x2000;
  if (_receiveBufferFull) data |= 0x4000;
  if (_receiveBufferOverrun) data |= 0x8000;

  return data;
}

void Uart::WriteSerdatRegister(uint16_t data)
{
  _transmitBuffer = data & 0x3ff;
  _transmitBufferEmpty = false;

  if (_transmitBuffer != 0)
  {
    CopyTransmitBufferToShiftRegister();
  }
}

void Uart::WriteSerperRegister(uint16_t data)
{
  _serper = data;  
}

uint32_t Uart::GetTransmitDoneTime()
{
  int bitsToTransfer = 2 + (Is8BitMode() ? 8 : 9);
  uint32_t cyclesPerBit = GetBitPeriod() + 1;

  return cyclesPerBit * bitsToTransfer;  
}

void Uart::CopyTransmitBufferToShiftRegister()
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

void Uart::CopyReceiveShiftRegisterToBuffer()
{
  _receiveBuffer = _receiveShiftRegister;
  _receiveBufferFull = true;  
  wintreq_direct(0x8400, 0xdff09c, true); // RBF interrupt
}

void Uart::NotifyInterruptRequestBitsChanged(uint16_t intreq)
{
  // Clear only, or is it directly wired?
  // HRM says overrun is also mirrored from intreq? How?
  _receiveBufferFull = (intreq & 0x0800) == 0x0800;  
  if (!_receiveBufferFull)
  {
    _receiveBufferOverrun = false;
  }
}

void Uart::InstallIOHandlers()
{
  memorySetIoWriteStub(0x030, wserdat);
  memorySetIoWriteStub(0x32, wserper);
  memorySetIoReadStub(0x018, rserdat);
}

void Uart::ClearState()
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

void Uart::LoadState(FILE *F)
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

void Uart::SaveState(FILE *F)
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

bool Uart::Is8BitMode()
{
  return (_serper & 0x8000) == 0;
}

uint16_t Uart::GetBitPeriod()
{
  return _serper & 0x3fff;
}

void Uart::OpenOutputFile()
{
  if (_outputFile == nullptr)
  {
    _outputFile = fopen(_outputFileName.c_str(), "wb");
  }
}

void Uart::CloseOutputFile()
{
  if (_outputFile != nullptr)
  {
    fclose(_outputFile);
    _outputFile = nullptr;
  }  
}

void Uart::EndOfLine()
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

void Uart::EndOfFrame()
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

void Uart::EmulationStart()
{
  InstallIOHandlers();
}

void Uart::EmulationStop()
{
  CloseOutputFile();
}

Uart::Uart()
  : _outputFile(nullptr)
{
  char tempFileName[256];
  _core.Fileops->GetGenericFileName(tempFileName, "WinFellow", "uart_output.bin");
  _outputFileName = tempFileName;
  ClearState();
}

Uart::~Uart()
{
  CloseOutputFile();
}

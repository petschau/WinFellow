#include "uart.h"
#include "fellow/scheduler/Scheduler.h"
#include "FMEM.H"
#include "interrupt.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

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
    _transmitDoneTime = GetTransmitDoneTime() + scheduler.GetFrameCycle();

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
  _transmitDoneTime = SchedulerEvent::EventDisableCycle;

  _receiveBuffer = 0;
  _receiveShiftRegister = 0;
  _receiveBufferFull = false;
  _receiveBufferOverrun = false;
  _receiveDoneTime = SchedulerEvent::EventDisableCycle;
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

  if (_transmitDoneTime <= scheduler.GetFrameCycle())
  {
    _transmitShiftRegisterEmpty = true;
    _transmitDoneTime = SchedulerEvent::EventDisableCycle;

    if (!_transmitBufferEmpty)
    {
      CopyTransmitBufferToShiftRegister();
    }
  }

  if (_receiveDoneTime <= scheduler.GetFrameCycle())
  {
    _receiveDoneTime = SchedulerEvent::EventDisableCycle;

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
  if (_transmitDoneTime != SchedulerEvent::EventDisableCycle)
  {
    _transmitDoneTime -= scheduler.GetCyclesInFrame();
    if ((int)_transmitDoneTime < 0)
    {
      _transmitDoneTime = 0;
    }
  }
  if (_receiveDoneTime != SchedulerEvent::EventDisableCycle)
  {
    _receiveDoneTime -= scheduler.GetCyclesInFrame();
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

void UART::Startup()
{
  char tempFileName[256];
  Service->Fileops.GetGenericFileName(tempFileName, "WinFellow", "uart_output.bin");
  _outputFileName = tempFileName;
}

void UART::Shutdown()
{
}

UART::UART() : _outputFile(nullptr)
{
  ClearState();
}

UART::~UART()
{
  CloseOutputFile();
}

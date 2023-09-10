#include "Graphics.h"

#include "BUS.H"

Graphics GraphicsContext;

void Graphics::Commit(ULO untilRasterY, ULO untilRasterX)
{
  if (GraphicsContext.Logger.IsLogEnabled())
  {
    GraphicsContext.Logger.Log(untilRasterY, untilRasterX*2+1, "Commit:\n-------------------------\n"); 
  }
  _queue.Run(untilRasterY*GraphicsEventQueue::GetCylindersPerLine() + untilRasterX*2 + 1);
}

void Graphics::InitializeEventQueue(void)
{
  _queue.Clear();

  DIWXStateMachine.InitializeEvent(&_queue);
  DIWYStateMachine.InitializeEvent(&_queue);
  DDFStateMachine.InitializeEvent(&_queue);
  BitplaneDMA.InitializeEvent(&_queue);
  PixelSerializer.InitializeEvent(&_queue);
}

void Graphics::EndOfFrame(void)
{
  DIWXStateMachine.EndOfFrame();
  DIWYStateMachine.EndOfFrame();
  DDFStateMachine.EndOfFrame();
  BitplaneDMA.EndOfFrame();
  PixelSerializer.EndOfFrame();
}

void Graphics::SoftReset(void)
{
  DIWXStateMachine.SoftReset();
  DIWYStateMachine.SoftReset();
  DDFStateMachine.SoftReset();
  PixelSerializer.SoftReset();
}

void Graphics::HardReset(void)
{
  DIWXStateMachine.HardReset();
  DIWYStateMachine.HardReset();
  DDFStateMachine.HardReset();
  PixelSerializer.HardReset();
}

void Graphics::EmulationStart(void)
{
  DIWXStateMachine.EmulationStart();
  DIWYStateMachine.EmulationStart();
  DDFStateMachine.EmulationStart();
  PixelSerializer.EmulationStart();
}

void Graphics::EmulationStop(void)
{
  DIWXStateMachine.EmulationStop();
  DIWYStateMachine.EmulationStop();
  DDFStateMachine.EmulationStop();
  PixelSerializer.EmulationStop();
}

void Graphics::Startup(void)
{
  DIWXStateMachine.Startup();
  DIWYStateMachine.Startup();
  DDFStateMachine.Startup();
  PixelSerializer.Startup();

  InitializeEventQueue();
}

void Graphics::Shutdown(void)
{
  DIWXStateMachine.Shutdown();
  DIWYStateMachine.Shutdown();
  DDFStateMachine.Shutdown();
  PixelSerializer.Shutdown();
  Logger.Shutdown();
}


#include "Graphics.h"

#ifdef GRAPH2

#include "CopperNew.h"
#include "BUS.H"

Graphics GraphicsContext;

void Graphics::Commit(ULO untilRasterY, ULO untilRasterX)
{
  _queue.Run(untilRasterY*BUS_CYCLE_PER_LINE + untilRasterX);
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
  BitplaneDMA.SoftReset();
  PixelSerializer.SoftReset();
}

void Graphics::HardReset(void)
{
  DIWXStateMachine.HardReset();
  DIWYStateMachine.HardReset();
  DDFStateMachine.HardReset();
  BitplaneDMA.HardReset();
  PixelSerializer.HardReset();
}

void Graphics::EmulationStart(void)
{
  DIWXStateMachine.EmulationStart();
  DIWYStateMachine.EmulationStart();
  DDFStateMachine.EmulationStart();
  BitplaneDMA.EmulationStart();
  PixelSerializer.EmulationStart();
  Sprites.EmulationStart();
}

void Graphics::EmulationStop(void)
{
  DIWXStateMachine.EmulationStop();
  DIWYStateMachine.EmulationStop();
  DDFStateMachine.EmulationStop();
  BitplaneDMA.EmulationStop();
  PixelSerializer.EmulationStop();
}

void Graphics::Startup(void)
{
  DIWXStateMachine.Startup();
  DIWYStateMachine.Startup();
  DDFStateMachine.Startup();
  BitplaneDMA.Startup();
  PixelSerializer.Startup();
  Copper_Startup();

  InitializeEventQueue();
}

void Graphics::Shutdown(void)
{
  DIWXStateMachine.Shutdown();
  DIWYStateMachine.Shutdown();
  DDFStateMachine.Shutdown();
  BitplaneDMA.Shutdown();
  PixelSerializer.Shutdown();
  Copper_Shutdown();
}

#endif

#pragma once

#include "Defs.h"

#include "GraphicsEventQueue.h"
#include "Planar2ChunkyDecoder.h"
#include "BitplaneDMA.h"
#include "PixelSerializer.h"
#include "BitplaneDraw.h"
#include "CycleExactSprites.h"
#include "DIWXStateMachine.h"
#include "DIWYStateMachine.h"
#include "DDFStateMachine.h"
#include "Logger.h"

class Graphics
{
private:
  GraphicsEventQueue _queue;

  void InitializeEventQueue();
  void InitializeDIWXEvent();
  void InitializeDIWYEvent();
  void InitializeDDFEvent();
  void InitializeBitplaneDMAEvent();
  void InitializePixelSerializerEvent();

public:
  DIWXStateMachine DIWXStateMachine;
  DIWYStateMachine DIWYStateMachine;
  DDFStateMachine DDFStateMachine;
  BitplaneDMA BitplaneDMA;
  PixelSerializer PixelSerializer;
  Planar2ChunkyDecoder Planar2ChunkyDecoder;
  BitplaneDraw BitplaneDraw;
  Logger Logger;

  void Commit(uint32_t untilRasterY, uint32_t untilRasterX);

  void EndOfFrame();
  void SoftReset();
  void HardReset();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();
};

extern Graphics GraphicsContext;

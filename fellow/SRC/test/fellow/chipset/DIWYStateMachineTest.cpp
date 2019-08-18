#include "test/catch/catch.hpp"
#include "fellow/chipset/DIWYStateMachine.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/scheduler/Scheduler.h"
#undef new

static void TestDiwYInitalizeEmptyBusQueue()
{
  scheduler.InitializeQueue();
  scheduler.ClearQueue();
}

static void TestDiwYInitalizeCycleExact()
{
  chipset_info.IsCycleExact = true;
}

static void TestDiwYHandleNextEvent()
{
  scheduler.HandleNextEvent();
}

static void TestDiwYInitialize(ULO startLine, ULO stopLine)
{
  diwy_state_machine.Startup();
  diwy_state_machine.SetStartLine(startLine);
  diwy_state_machine.SetStopLine(stopLine);
  diwy_state_machine.EndOfFrame();
}

static void TestDiwYSetBusCycle(ULO line, ULO cycle)
{
  scheduler.SetFrameCycle(line * scheduler.GetCyclesInLine() + cycle);
}

TEST_CASE("DIWYStateMachine is working", "[diwystatemachine]")
{
  TestDiwYInitalizeEmptyBusQueue();
  TestDiwYInitalizeCycleExact();

  SECTION("Startup() clears all state and initialize state for running from the beginning of a frame and does not put event on queue")
  {
    diwy_state_machine.SetStartLine(300);
    diwy_state_machine.SetStopLine(302);
    diwy_state_machine.GetCurrentState().Set(DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE, SHResTimestamp(302, 0));
    diwy_state_machine.GetNextState().Set(DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE, SHResTimestamp(MAXINT, 0));

    diwy_state_machine.Startup();

    REQUIRE(diwy_state_machine.GetStartLine() == 0);
    REQUIRE(diwy_state_machine.GetStopLine() == 0);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 0);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 0);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 1);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(&diwyEvent != scheduler.PeekNextEvent());
  }

  SECTION("EndOfFrame() initialize for running from the beginning of a frame using existing start line, and puts event on queue")
  {
    diwy_state_machine.Startup();
    diwy_state_machine.SetStartLine(47);
    diwy_state_machine.SetStopLine(309);
    diwy_state_machine.GetCurrentState().Set(DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE, SHResTimestamp(309, 0));
    diwy_state_machine.GetNextState().Set(DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE, SHResTimestamp(MAXINT, 0));

    diwy_state_machine.EndOfFrame();

    REQUIRE(diwy_state_machine.GetStartLine() == 47);
    REQUIRE(diwy_state_machine.GetStopLine() == 309);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 0);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 47);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 47 * scheduler.GetCyclesInLine());
  }

  SECTION("Handler() when waiting for start line, event arrives and stop line is higher, it sets state to waiting for stop line")
  {
    TestDiwYInitialize(50, 60);

    TestDiwYHandleNextEvent();

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 50);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 60 * scheduler.GetCyclesInLine());
  }

  SECTION("Handler() when waiting for start line, event arrives and stop line is less than start line, it sets state to waiting for stop line and disables the event")
  {
    TestDiwYInitialize(50, 40);

    TestDiwYHandleNextEvent();

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 50);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == MAXINT);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(scheduler.PeekNextEvent() == nullptr);
    REQUIRE(diwyEvent.cycle == SchedulerEvent::EventDisableCycle);
  }

  SECTION("Handler() when waiting for start line, event arrives and stop line is the same as start line, it sets state to waiting for stop line and puts event on queue one cycle later")
  {
    TestDiwYInitialize(50, 50);

    TestDiwYHandleNextEvent();

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 50);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 50);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 1);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 50 * scheduler.GetCyclesInLine() + 1);
  }

  SECTION("Handler() when waiting for stop line, event arrives and start line is less than stop line, it sets state to waiting for start line and disables the event")
  {
    TestDiwYInitialize(50, 60);

    TestDiwYHandleNextEvent(); // Wait for start -> wait for stop
    TestDiwYHandleNextEvent(); // Wait for stop -> wait for start

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == MAXINT);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(scheduler.PeekNextEvent() == nullptr);
    REQUIRE(diwyEvent.cycle == SchedulerEvent::EventDisableCycle);
  }

  SECTION("Handler() when waiting for stop line, event arrives and start line the same as stop line, it sets state to waiting for start line and puts an event on the queue one cycle later")
  {
    TestDiwYInitialize(50, 60);

    TestDiwYHandleNextEvent(); // Step past wait for start 50 -> wait for stop 60
    TestDiwYSetBusCycle(55, 132);
    diwy_state_machine.NotifyDIWStrtChanged(60);
    TestDiwYHandleNextEvent(); // Step past wait for stop 60 -> wait for start 60

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 1);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 60 * scheduler.GetCyclesInLine() + 1);
  }

  SECTION("Handler() when waiting for stop line, event arrives and start line is higher than stop line, it sets state to waiting for start line")
  {
    TestDiwYInitialize(50, 60);

    TestDiwYHandleNextEvent(); // Step past wait for start 50 -> wait for stop 60
    TestDiwYSetBusCycle(55, 132);
    diwy_state_machine.NotifyDIWStrtChanged(70);
    TestDiwYHandleNextEvent(); // Step past wait for stop 60 -> wait for start 70

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 70);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 70 * scheduler.GetCyclesInLine());
  }

  SECTION("NotifyDIWStrtChanged() when waiting for start line and start line changed, it should update the change line on the next state and reissue event on queue")
  {
    TestDiwYInitialize(60, 100);

    TestDiwYSetBusCycle(40, 134);
    diwy_state_machine.NotifyDIWStrtChanged(48);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 0);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 48);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 48 * scheduler.GetCyclesInLine());
  }

  SECTION("NotifyDIWStrtChanged() when waiting for start line and start line changed to a line that is passed, it should update the change line on the next state to infinity and disable the event")
  {
    TestDiwYInitialize(60, 100);

    TestDiwYSetBusCycle(50, 134);
    diwy_state_machine.NotifyDIWStrtChanged(32);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 0);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == MAXINT);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(scheduler.PeekNextEvent() == nullptr);
    REQUIRE(diwyEvent.cycle == SchedulerEvent::EventDisableCycle);
  }

  SECTION("NotifyDIWStrtChanged() when waiting for start line and start line changed to the current line, it should update the change cycle on the next state to the current cycle + 1")
  {
    TestDiwYInitialize(60, 100);

    TestDiwYSetBusCycle(50, 150);
    diwy_state_machine.NotifyDIWStrtChanged(50);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 0);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 50);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 151);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 50 * scheduler.GetCyclesInLine() + 151);
  }

  SECTION("NotifyDIWStrtChanged() when waiting for start line and start line changed to the current line on the last cycle of the line, it should update the change cycle on the next state to infinity and disable the event")
  {
    TestDiwYInitialize(60, 100);
    
    TestDiwYSetBusCycle(48, 226);
    diwy_state_machine.NotifyDIWStrtChanged(48);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 0);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == MAXINT);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(scheduler.PeekNextEvent() == nullptr);
    REQUIRE(diwyEvent.cycle == SchedulerEvent::EventDisableCycle);
  }

  SECTION("NotifyDIWStrtChanged() when waiting for start line and start line changed to a lower value on the exact cycle the current state change is scheduled, it should switch state before finding next state based on the new start line")
  {
    TestDiwYInitialize(60, 100);

    TestDiwYSetBusCycle(60, 0);
    diwy_state_machine.NotifyDIWStrtChanged(48);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 100);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 100 * scheduler.GetCyclesInLine());
  }

  SECTION("NotifyDIWStrtChanged() when waiting for start line and start line changed to a higher value on the exact cycle the current state change is scheduled, it switch state before finding next state based on the new start line")
  {
    TestDiwYInitialize(60, 70);

    TestDiwYSetBusCycle(60, 0);
    diwy_state_machine.NotifyDIWStrtChanged(65);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 70);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 70 * scheduler.GetCyclesInLine());
  }

  SECTION("NotifyDIWStopChanged() when waiting for stop line and stop line changed, it should update the change line on the next state and reissue event on queue")
  {
    TestDiwYInitialize(60, 255);
    TestDiwYHandleNextEvent();

    TestDiwYSetBusCycle(70, 134);
    diwy_state_machine.NotifyDIWStopChanged(240);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 240);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 240 * scheduler.GetCyclesInLine());
  }

  SECTION("NotifyDIWStopChanged() when waiting for stop line and stop line changed to a line that is passed, it should update the change line on the next state to infinity and disable the event")
  {
    TestDiwYInitialize(60, 300);
    TestDiwYHandleNextEvent(); // Step past waiting for start line -> waiting for stop line at 300

    TestDiwYSetBusCycle(260, 139);
    diwy_state_machine.NotifyDIWStopChanged(240); // At line 260, change stop line to 240

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == MAXINT);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(scheduler.PeekNextEvent() == nullptr);
    REQUIRE(diwyEvent.cycle == SchedulerEvent::EventDisableCycle);
  }

  SECTION("NotifyDIWStopChanged() when waiting for stop line and stop line changed to the current line, it should update the change cycle on the next state to the current cycle + 1")
  {
    TestDiwYInitialize(60, 310);
    TestDiwYHandleNextEvent();

    TestDiwYSetBusCycle(240, 150);
    diwy_state_machine.NotifyDIWStopChanged(240);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == 240);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 151);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(&diwyEvent == scheduler.PeekNextEvent());
    REQUIRE(diwyEvent.cycle == 240 * scheduler.GetCyclesInLine() + 151);
  }

  SECTION("NotifyDIWStopChanged() when waiting for stop line and stop line changed to the current line on the last cycle of the line, it should update the change cycle on the next state to infinity and disable the event")
  {
    TestDiwYInitialize(60, 255);
    TestDiwYHandleNextEvent();

    TestDiwYSetBusCycle(240, 226);
    diwy_state_machine.NotifyDIWStopChanged(240);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 60);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == MAXINT);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == true);

    REQUIRE(scheduler.PeekNextEvent() == nullptr);
    REQUIRE(diwyEvent.cycle == SchedulerEvent::EventDisableCycle);
  }

  SECTION("NotifyDIWStopChanged() when waiting for stop line and stop line changed to a lower value on the exact cycle the current state change is scheduled, it should switch state before finding next state based on the new stop line")
  {
    TestDiwYInitialize(60, 272);
    TestDiwYHandleNextEvent();

    TestDiwYSetBusCycle(272, 0);
    diwy_state_machine.NotifyDIWStopChanged(264);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 272);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == MAXINT);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(scheduler.PeekNextEvent() == nullptr);
    REQUIRE(diwyEvent.cycle == SchedulerEvent::EventDisableCycle);
  }

  SECTION("NotifyDIWStopChanged() when waiting for stop line and stop line changed to a higher value on the exact cycle the current state change is scheduled, it switch state before finding next state based on the new stop line")
  {
    TestDiwYInitialize(60, 240);
    TestDiwYHandleNextEvent();

    TestDiwYSetBusCycle(240, 0);
    diwy_state_machine.NotifyDIWStopChanged(272);

    REQUIRE(diwy_state_machine.GetCurrentState().State == DIWYStates::DIWY_STATE_WAITING_FOR_START_LINE);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Line == 240);
    REQUIRE(diwy_state_machine.GetCurrentState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.GetNextState().State == DIWYStates::DIWY_STATE_WAITING_FOR_STOP_LINE);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Line == MAXINT);
    REQUIRE(diwy_state_machine.GetNextState().ChangeTime.Pixel == 0);

    REQUIRE(diwy_state_machine.IsVisible() == false);

    REQUIRE(scheduler.PeekNextEvent() == nullptr);
    REQUIRE(diwyEvent.cycle == SchedulerEvent::EventDisableCycle);
  }
}

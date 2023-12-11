
/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2023, SparkFun Electronics Inc.  All rights reserved.
 * This software includes information which is proprietary to and a
 * trade secret of SparkFun Electronics Inc.  It is not to be disclosed
 * to anyone outside of this organization. Reproduction by any means
 * whatsoever is  prohibited without express written permission.
 *
 *---------------------------------------------------------------------------------
 */
// Implements onboard LED updates in an ESP32 task

#include "Arduino.h"

#include "sfeDLBoard.h"
#include "sfeDLLed.h"

// task handle
static TaskHandle_t hTaskLED = NULL;

// "Command Queue"
QueueHandle_t hCmdQueue = NULL;

// size of our queue
const uint16_t kLEDCmdQueueSize = 12;
const uint16_t kLEDCmdQueueWait = 5;

// Time for flashing the LED
xTimerHandle hTimer;
const uint16_t kTimerPeriod = 100;

// A task needs a Stack - let's set that size
#define kStackSize 1024
#define kActivityDelay 60

#define kLedColorOrder GRB
#define kLedChipset WS2812
#define kLEDBrightness 20

_sfeLED &sfeLED = _sfeLED::get();

//------------------------------------------------------------------------------
// Callback for the FreeRTOS timer -- used to blink LED

static void _sfeLED_TimerCallback(xTimerHandle pxTimer)
{
    sfeLED._timerCB();
}

//--------------------------------------------------------------------------------
// task event loop - standard C static method - for FreeRTOS

static void _sfeLED_TaskProcessing(void *parameter)
{

    if (hCmdQueue == NULL)
        return;

    // startup delay
    vTaskDelay(50);

    _sfeLED::cmdStruct_t theCommand;

    while (true)
    {
        // get next command from the queue - block if needed
        if (xQueueReceive(hCmdQueue, &theCommand, portMAX_DELAY) != pdPASS)
            continue; // should never get here...

        // Flash commands can be compressed - skip dups in queue - if the next command is flash and this
        // command is flash, skip it.

        if (theCommand.type == _sfeLED::kCmdFlash)
        {
            _sfeLED::cmdStruct_t thePeek;

            if (xQueuePeek(hCmdQueue, &thePeek, (TickType_t)5))
            {
                // skip the current flash?
                if (thePeek.type == _sfeLED::kCmdFlash)
                    continue;
            }
        }
        // send the new command to the event processor -- in the object
        sfeLED._eventCB(theCommand);
    }
}

//---------------------------------------------------------
// _sfeLED implementation
//---------------------------------------------------------

_sfeLED::_sfeLED() : _current{0}, _isInitialized{false}, _blinkOn{false}, _disabled{false}
{
    _colorStack[0] = {_sfeLED::Black, 0};
}

//---------------------------------------------------------
bool _sfeLED::initialize(void)
{

    // Begin setup - turn on board LED during setup.
    pinMode(kDLBoardLEDRGBBuiltin, OUTPUT);

    // Create a timer, which is used to drive the user experience.
    hTimer = xTimerCreate("ledtimer", kTimerPeriod / portTICK_RATE_MS, pdTRUE, (void *)0, _sfeLED_TimerCallback);
    if (hTimer == NULL)
    {
        // no timer - whoa
        Serial.println("[WARNING] - failed to create LED timer");
        return false;
    }

    hCmdQueue = xQueueCreate(kLEDCmdQueueSize, sizeof(cmdStruct_t));

    if (hCmdQueue == NULL)
    {
        Serial.println("[ERROR] - LED startup - queue failed");
        xTimerDelete(hTimer, 100);
        hTimer = NULL;
        return false;
    }

    // Event processing task
    BaseType_t xReturnValue = xTaskCreate(_sfeLED_TaskProcessing, // Event processing task function
                                          "eventProc",            // String with name of task.
                                          kStackSize,             // Stack size in 32 bit words.
                                          NULL,                   // Parameter passed as input of the task
                                          1,                      // Priority of the task.
                                          &hTaskLED);             // Task handle.

    if (xReturnValue != pdPASS)
    {
        hTaskLED = NULL;
        Serial.println("[ERROR] - Failure to start event processing task. Halting");

        xTimerDelete(hTimer, 100);
        hTimer = NULL;
        vQueueDelete(hCmdQueue);
        hCmdQueue = NULL;
        return false;
    }

    FastLED.addLeds<kLedChipset, kDLBoardLEDRGBBuiltin, kLedColorOrder>(&_theLED, 1).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(kLEDBrightness);

    _isInitialized = true;

    refresh();

    return true;
}

//------------------------------------------------------------------------------
// Callback for the FreeRTOS timer -- used to blink LED
void _sfeLED::_timerCB(void)
{
    _theLED = _blinkOn ? _sfeLED::Black : _colorStack[_current].color;
    FastLED.show();

    _blinkOn = !_blinkOn;
}

//---------------------------------------------------------
// Command "event" callback -

void _sfeLED::_eventCB(cmdStruct_t &theCommand)
{
    switch (theCommand.type)
    {

    // LED on - new state
    case kCmdOn:
        pushState(theCommand.data);
        break;

    // LED off - pop current state
    case kCmdOff:
        popState();
        break;

    // quick flash of LED
    case kCmdFlash:
        pushState(theCommand.data);
        vTaskDelay(kActivityDelay / portTICK_RATE_MS);
        popState();
        break;

    // Change the blink rate
    case kCmdBlink:
        _colorStack[_current].ticks = theCommand.data.ticks;
        update();
        break;

    case kCmdReset:
        _current = 0;
        update();
        break;

    case kCmdUpdate:
        update();
        break;

    case kCmdNone:
    default:
        break;
    }
}
//---------------------------------------------------------
// private.
//---------------------------------------------------------

//---------------------------------------------------------
// Update the LED UX to reflect current state in stack

void _sfeLED::update(void)
{
    if (!_isInitialized)
        return;

    _theLED = _colorStack[_current].color;
    FastLED.show();

    // check blink state - blink on, blink off?
    if (_colorStack[_current].ticks > 0)
    {
        xTimerChangePeriod(hTimer, _colorStack[_current].ticks / portTICK_RATE_MS, 10);
        xTimerReset(hTimer, 10);
    }
    else
        xTimerStop(hTimer, 10);
}

//---------------------------------------------------------
void _sfeLED::popState(void)
{
    if (_current > 0)
    {
        _current--;
        update();
    }
}
//---------------------------------------------------------
bool _sfeLED::pushState(colorState_t &newState)
{

    if (_current > kStackSize - 2)
        return false; // no room

    // add the new state ..
    _current++;
    _colorStack[_current] = newState;
    update();

    return true;
}

//---------------------------------------------------------
// queue up a command

void _sfeLED::queueCommand(cmdType_t command, sfeLEDColor_t color, uint32_t ticks)
{

    if (!_isInitialized)
        return;

    cmdStruct_t theCommand = {command, {color, ticks}};

    if (xQueueSend(hCmdQueue, (void *)&theCommand, kLEDCmdQueueWait / portTICK_RATE_MS) != pdPASS)
        Serial.println("[WARNING] - LED queue overflow"); // TODO -- DO WE CARE
}
//---------------------------------------------------------
// public interface methods.
//---------------------------------------------------------
// Flash the  LED

void _sfeLED::flash(sfeLEDColor_t color)
{
    if (_disabled)
        return;

    queueCommand(kCmdFlash, color);
}

//---------------------------------------------------------
// LED Off - end current state

void _sfeLED::off(void)
{
    if (_disabled)
        return;

    queueCommand(kCmdOff);
}

//---------------------------------------------------------
// LED on - new state

void _sfeLED::on(sfeLEDColor_t color)
{
    if (_disabled)
        return;

    queueCommand(kCmdOn, color);
}

//---------------------------------------------------------
// Blink - change the timer value of the current color

void _sfeLED::blink(uint timeout)
{
    if (_disabled)
        return;

    queueCommand(kCmdBlink, 0, timeout);
}

//---------------------------------------------------------
// Blink - change state, start blinking

void _sfeLED::blink(sfeLEDColor_t color, uint timeout)
{
    if (_disabled)
        return;

    queueCommand(kCmdOn, color, timeout);
}
//---------------------------------------------------------
void _sfeLED::stop(bool turnoff)
{
    if (_disabled)
        return;

    queueCommand(turnoff ? kCmdOff : kCmdBlink);
}
//---------------------------------------------------------
// refresh
void _sfeLED::refresh(void)
{
    if (_disabled)
        return;

    queueCommand(kCmdUpdate);
}

//---------------------------------------------------------
// Enable/Disable?
void _sfeLED::setDisabled(bool bDisable)
{

    if (bDisable == _disabled)
        return;

    _disabled = bDisable;

    // reset???
    if (bDisable)
        queueCommand(kCmdReset);
}

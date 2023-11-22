
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

// Our event group handle
static EventGroupHandle_t hEventGroup = NULL;

// task handle
static TaskHandle_t hTaskLED = NULL;

// Binary semaphore/mutex to manage stack access
SemaphoreHandle_t  hMutex = NULL;

// mutex wait times
#define kMutexPeriod 10
#define kMutexPeriodFast 1

// Time for flashing the LED
xTimerHandle hTimer;
#define kTimerPeriod 100

// Event Types (these map to bits -- for XEventGroup calls)
typedef enum
{
    kEventNull = 0UL,
    kEventActvity = (1UL << 0UL),
    kEventPending = (1UL << 1UL),
    kEventStartup = (1UL << 2UL),
    kEventBusy = (1UL << 3UL),
    kEventEditing = (1UL << 4UL),
    kEventOff = (1UL << 5UL)
} UXEvent_t;

#define kMaxEvent 6
#define kEventAllMask ~(0xFFFFFFFFUL << kMaxEvent)

// A task needs a Stack - let's set that size
#define kStackSize 1024
#define kActivityDelay 100

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
// task event loop

static void _sfeLED_TaskProcessing(void *parameter)
{
    // startup delay
    vTaskDelay(50);
    uint32_t eventBits;

    while (true)
    {
        eventBits = (uint32_t)xEventGroupWaitBits(hEventGroup, kEventAllMask, pdTRUE, pdFALSE, portMAX_DELAY);

        sfeLED._eventCB(eventBits);
    }
}

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
        Serial.println("[WARNING] - failed to create LED timer");
    }

    hEventGroup = xEventGroupCreate();
    if (!hEventGroup)
        return false;

    // create mutex
    hMutex = xSemaphoreCreateBinary();
    if (!hMutex)
    {
        Serial.println("[ERROR] - LED start lock failure");
        return false;
    }
    // make the mutex avaialble
    xSemaphoreGive(hMutex);

    // Event processing task
    BaseType_t xReturnValue = xTaskCreate(_sfeLED_TaskProcessing, // Event processing task functoin
                                          "eventProc",            // String with name of task.
                                          kStackSize,             // Stack size in 32 bit words.
                                          NULL,                   // Parameter passed as input of the task
                                          1,                      // Priority of the task.
                                          &hTaskLED);             // Task handle.

    if (xReturnValue != pdPASS)
    {
        hTaskLED = NULL;
        Serial.println("[ERROR] - Failure to start event processing task. Halting");
        return false;
    }

    FastLED.addLeds<kLedChipset, kDLBoardLEDRGBBuiltin, kLedColorOrder>(&_theLED, 1).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(kLEDBrightness);

    _isInitialized = true;
    update();
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
void _sfeLED::_eventCB(uint32_t event)
{
    if (event & kEventActvity == kEventActvity)
    {
        update();
        vTaskDelay(kActivityDelay / portTICK_RATE_MS);
        off();
    }
}
//---------------------------------------------------------
// private.
//---------------------------------------------------------

void _sfeLED::start_blink(void)
{
    if (!_isInitialized || _colorStack[_current].ticks == 0)
        return;

    xTimerChangePeriod(hTimer, _colorStack[_current].ticks / portTICK_RATE_MS, 10);
    xTimerReset(hTimer, 10);
}

//---------------------------------------------------------
void _sfeLED::stop_blink(void)
{
    if (xTimerStop(hTimer, 10) != pdPASS)
        Serial.println("Error stop LED timer");
}

//---------------------------------------------------------
void _sfeLED::update(void)
{
    if (!_isInitialized || _disabled)
        return;

    // get access to the stack
    if (xSemaphoreTake(hMutex, kMutexPeriod/portTICK_RATE_MS) != pdTRUE)
        return;// no joy on take

    _theLED = _colorStack[_current].color;
    FastLED.show();

    if (_colorStack[_current].ticks > 0)
        start_blink();

    xSemaphoreGive(hMutex);
}

//---------------------------------------------------------
void _sfeLED::popState(void)
{
    // get access to the stack
    if (xSemaphoreTake(hMutex, kMutexPeriod/portTICK_RATE_MS) != pdTRUE)
        return; // no joy on take

    if (_current >  0)
    {
        if (_colorStack[_current].ticks > 0)
            stop_blink();

        _current--;
    }
    xSemaphoreGive(hMutex);
}
//---------------------------------------------------------
bool _sfeLED::pushState(sfeLEDColor_t color)
{
    bool status = false;
    // get access to the stack
    if (xSemaphoreTake(hMutex, kMutexPeriodFast/portTICK_RATE_MS) != pdTRUE)
        return false; // no joy on take

    if (_current < kStackSize - 1)
    {
        _current++;
        _colorStack[_current] = {color, 0};
        status = true;
    }         
    xSemaphoreGive(hMutex);

    return status;
}
//---------------------------------------------------------
// public
//---------------------------------------------------------

void _sfeLED::flash(sfeLEDColor_t color)
{
    if (_disabled)
        return;

    // push new color, set activity 
    if ( pushState(color) )
        xEventGroupSetBits(hEventGroup, kEventActvity);
}

//---------------------------------------------------------
void _sfeLED::off(void)
{
    if ( _disabled)
        return;

    popState();
    update();
}

//---------------------------------------------------------
void _sfeLED::on(sfeLEDColor_t color)
{

    if (_disabled)
        return;

    if (pushState(color))
        update();
}

//---------------------------------------------------------
void _sfeLED::blink(uint timeout)
{
    // at off state?
    if (xSemaphoreTake(hMutex, kMutexPeriodFast/portTICK_RATE_MS) != pdTRUE)
        return; // no joy on take

    if (_current > 0 && _isInitialized && !_disabled)
    {
        _colorStack[_current].ticks = timeout;
        _blinkOn = true;
        start_blink();
    }
    xSemaphoreGive(hMutex);

}
//---------------------------------------------------------
void _sfeLED::blink(sfeLEDColor_t color, uint timeout)
{
    if (_disabled)
        return;

    on(color);
    blink(timeout);
}
//---------------------------------------------------------
void _sfeLED::stop(bool turnoff)
{
    if (_disabled)
        return;

    stop_blink();

    if (turnoff)
        off();
}

//---------------------------------------------------------
void _sfeLED::setDisabled(bool bDisable)
{

    if (bDisable == _disabled)
        return;

    if (bDisable && _current > 0 && _isInitialized) 
    {
        off();
        _current = 0;
    }
    

    _disabled = bDisable;
}


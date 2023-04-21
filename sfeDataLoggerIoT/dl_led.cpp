
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
// Impelements onboard LED updates in an ESP32 task

#include "Arduino.h"

#include "dl_led.h"

// quiet a damn prama message - silly
#define FASTLED_INTERNAL
#include <FastLED.h>

// Our event group handle
static EventGroupHandle_t  hEventGroup = NULL;

// task handle  
static TaskHandle_t hTaskLED = NULL;   

// Event Types (these map to bits -- for XEventGroup calls)
typedef enum {
    kEventNull          = 0UL,
    kEventActvity       = (1UL << 0UL),
    kEventPending       = (1UL << 1UL),
    kEventStartup       = (1UL << 2UL),
    kEventBusy          = (1UL << 3UL),
    kEventEditing       = (1UL << 4UL),    
    kEventOff           = (1UL << 5UL)        
}UXEvent_t;

#define kMaxEvent 7
#define kEventAllMask  ~(0xFFFFFFFFUL << kMaxEvent )

// A task needs a Stack - let's set that size
#define kStackSize  1024
#define kActivityDelay 100


#define kLedRGBLedPin       26 //Pin for the datalogger
#define kLedColorOrder      GRB
#define kLedChipset         WS2812
#define kLedNumLed          1
#define kLEDBrightness      20

static CRGB leds[kLedNumLed];


static bool isEnabled = false;

//--------------------------------------------------------------------------------
// helpers 
static void _ledBlue(void)
{
    leds[0] = CRGB::Blue;
    FastLED.show();
}
//--------------------------------------------------------------------------------
static void _ledGreen(void)
{
    leds[0] = CRGB::Green;
    FastLED.show();
}

//--------------------------------------------------------------------------------
static void _ledYellow(void)
{
    leds[0] = CRGB::Yellow;
    FastLED.show();
}
//--------------------------------------------------------------------------------
static void _ledGray(void)
{
    leds[0] = CRGB::LightSlateGray;
    FastLED.show();
}

//--------------------------------------------------------------------------------
static void _ledOff(void)
{
    leds[0] = CRGB::Black;
    FastLED.show();
}

//--------------------------------------------------------------------------------
// task event loop

static void taskLEDProcessing(void *parameter){


	// startup delay
	vTaskDelay(50);
    UXEvent_t eventBits;

    while(true)
    {

    	eventBits = (UXEvent_t)xEventGroupWaitBits(hEventGroup, kEventAllMask, pdTRUE, pdFALSE, portMAX_DELAY) ;

    	if (eventBits & kEventActvity  == kEventActvity)
    	{
            _ledBlue();
    		vTaskDelay(kActivityDelay/portTICK_RATE_MS);
            _ledOff();

    	}
    	else if (eventBits & kEventStartup == kEventStartup)
            _ledGreen();

    	else if (eventBits &  kEventOff == kEventOff)
    	   _ledOff();

        else if (eventBits & kEventBusy == kEventBusy)
            _ledYellow();
        else if (eventBits & kEventEditing == kEventEditing)
            _ledGray();

    }
}

//--------------------------------------------------------------------------------
// Startup led color
//
// Note - will use immediate mode here - since event task isn't fully started 
void dl_ledStartup(bool immediate)
{
	if (!isEnabled)
		return;

    // event?
    if (!immediate)
    	xEventGroupSetBits(hEventGroup, kEventStartup);
    else 
        _ledGreen();
}

//--------------------------------------------------------------------------------
// led off 
void dl_ledOff(bool immediate)
{
	if (!isEnabled)
		return;

    // event driven?
    if (!immediate)
        xEventGroupSetBits(hEventGroup, kEventOff);
    else 
        _ledOff();

}

//--------------------------------------------------------------------------------
// led busy 
void dl_ledBusy(bool immediate)
{
    if (!isEnabled)
        return;

    // event driven?
    if (!immediate)
        xEventGroupSetBits(hEventGroup, kEventBusy);
    else 
        _ledYellow();

}
//--------------------------------------------------------------------------------
void dl_ledActivity(bool immediate)
{
	if (!isEnabled)
		return;

    // just do event driven
	xEventGroupSetBits(hEventGroup, kEventActvity);
}
//--------------------------------------------------------------------------------
void dl_ledEditing(bool immediate)
{
    if (!isEnabled)
        return;

        // event driven?
    if (!immediate)
        xEventGroupSetBits(hEventGroup, kEventEditing);
    else 
        _ledGray();

}
//--------------------------------------------------------------------------------
bool dl_ledInit(void)
{


	hEventGroup = xEventGroupCreate();
    if(!hEventGroup)
    	return false;

    // Event processing task
    BaseType_t xReturnValue = xTaskCreate(taskLEDProcessing,    // Event processing task functoin
                            "eventProc",                            // String with name of task.
                            kStackSize,             // Stack size in 32 bit words.
                            NULL,                                  // Parameter passed as input of the task
                            1,                                      // Priority of the task.
                            &hTaskLED);                         // Task handle.

    if(xReturnValue != pdPASS){
    	hTaskLED = NULL;
        Serial.println("[ERROR] - Failure to start event processing task. Halting");
        return false;
    }

    FastLED.addLeds<kLedChipset, kLedRGBLedPin, kLedColorOrder>(leds, kLedNumLed).setCorrection( TypicalLEDStrip );
  	FastLED.setBrightness( kLEDBrightness );

  	isEnabled=true;
    return true;
}
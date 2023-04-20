
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
    kEventOff           = (1UL << 3UL)        
}UXEvent_t;

#define kMaxEvent 5
#define kEventAllMask  ~(0xFFFFFFFFUL << kMaxEvent )

// A task needs a Stack - let's set that size
#define kStackSize  1024

#define kActivityDelay 100

static uint theOnBoardLEDPin = 0;


#define LED_PIN     26 //Pin 2 on Thing Plus C is connected to WS2812 LED
#define COLOR_ORDER GRB
#define CHIPSET     WS2812
#define NUM_LEDS    1

#define kLEDBrightness 20

static bool isEnabled = false;
static CRGB leds[NUM_LEDS];

static void taskLEDProcessing(void *parameter){


	// startup delay
	vTaskDelay(50);
    UXEvent_t eventBits;

    while(true)
    {

    	eventBits = (UXEvent_t)xEventGroupWaitBits(hEventGroup, kEventAllMask, pdTRUE, pdFALSE, portMAX_DELAY) ;

    	if (eventBits & kEventActvity  == kEventActvity)
    	{
    		leds[0] = CRGB::Blue;
    		FastLED.show();
    		vTaskDelay(kActivityDelay/portTICK_RATE_MS);
    		leds[0] = CRGB::Black;
    		FastLED.show();
    	}
    	else if (eventBits & kEventStartup == kEventStartup)
    	{
    		leds[0] = CRGB::Green;
    		FastLED.show();
    	}
    	else if (eventBits &  kEventOff == kEventOff)
    	{
    		leds[0] = CRGB::Black;
    		FastLED.show();
    	}

    }
}

void dl_ledStartup(void)
{
	if (!isEnabled)
		return;

	xEventGroupSetBits(hEventGroup, kEventStartup);
}
void dl_ledOff(void)
{
	if (!isEnabled)
		return;

	xEventGroupSetBits(hEventGroup, kEventOff);
}

void dl_ledActivity(void)
{
	if (!isEnabled)
		return;

	xEventGroupSetBits(hEventGroup, kEventActvity);
}


bool dl_ledInit(uint ledPIN)
{

	theOnBoardLEDPin = ledPIN;

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

    FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  	FastLED.setBrightness( kLEDBrightness );

  	isEnabled=true;
    return true;
}
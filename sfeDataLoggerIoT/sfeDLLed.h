
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

// LED Display for the datalogger - led display in a task
#pragma once

// quiet a damn pragma message - silly
#define FASTLED_INTERNAL
#include <FastLED.h>

// Some Handy Flash levels...
const uint16_t kLEDFlashSlow = 600;
const uint16_t kLEDFlashMedium = 200;
const uint16_t kLEDFlashFast = 80;

typedef uint32_t sfeLEDColor_t;
//---------------------------------------------------------------
class _sfeLED
{
  public:
    // this is a singleton
    static _sfeLED &get(void)
    {
        static _sfeLED instance;
        return instance;
    }

    //-------------------------------------------------------------------------
    // Delete copy and assignment constructors - b/c this is singleton.
    _sfeLED(_sfeLED const &) = delete;
    void operator=(_sfeLED const &) = delete;

    // colors
    static constexpr sfeLEDColor_t Black = 0x000000;
    static constexpr sfeLEDColor_t Blue = 0x0000FF;
    static constexpr sfeLEDColor_t Green = 0x008000;
    static constexpr sfeLEDColor_t Yellow = 0xFFFF00;
    static constexpr sfeLEDColor_t Red = 0xFF0000;
    static constexpr sfeLEDColor_t Gray = 0x808080;
    static constexpr sfeLEDColor_t LightGray = 0x778899;
    static constexpr sfeLEDColor_t Orange = 0xFFA500;
    static constexpr sfeLEDColor_t White = 0xFFFFFF;
    static constexpr sfeLEDColor_t Purple = 0x80008;

    typedef struct
    {
        sfeLEDColor_t color;
        uint32_t ticks;
    } colorState_t;

    // Command Type
    typedef enum
    {
        kCmdNone = 0,
        kCmdOn = (1 << 0),
        kCmdOff = (1 << 1),
        kCmdFlash = (1 << 2),
        kCmdBlink = (1 << 3),
        kCmdReset = (1 << 4),
        kCmdUpdate = (1 << 5)
    } cmdType_t;

    // Command struct
    typedef struct
    {
        cmdType_t type;
        colorState_t data;
    } cmdStruct_t;

    bool initialize(void);
    void on(sfeLEDColor_t color);
    void off(void);
    void blink(uint32_t);
    void blink(sfeLEDColor_t, uint32_t);
    void stop(bool off = true);
    void flash(sfeLEDColor_t color);
    void refresh(void);

    void _timerCB(void);
    void _eventCB(cmdStruct_t &);

    void setDisabled(bool bDisable);

    bool disabled(void)
    {
        return _disabled;
    }

  private:
    _sfeLED();

    void popState(void);
    bool pushState(colorState_t &);
    void update(void);
    void queueCommand(cmdType_t command, sfeLEDColor_t color = 0, uint32_t ticks = 0);

    static constexpr uint16_t kStackSize = 10;
    colorState_t _colorStack[kStackSize];

    int _current;
    bool _isInitialized;

    bool _blinkOn;

    CRGB _theLED;

    bool _disabled;
};
extern _sfeLED &sfeLED;
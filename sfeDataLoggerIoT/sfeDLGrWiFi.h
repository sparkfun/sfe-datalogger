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
#pragma once

#define IF_THIS_IS_ENABLED
#include "sfeDLGraphx.h"

// Icons
const int kIconWiFiHeight = 9;
const int kIconWiFiWidth = 13;

const uint8_t kIconsWiFi[4][26] = {
    // Off
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Poor
    {0x00, 0x00, 0x00, 0x00, 0x20, 0x90, 0xD0, 0x90, 0x20, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Good
    {0x00, 0x00, 0x10, 0x08, 0x24, 0x94, 0xD4, 0x94, 0x24, 0x08, 0x10, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // FULL
    {0x08, 0x04, 0x12, 0x09, 0x25, 0x95, 0xD5, 0x95, 0x25, 0x09, 0x12, 0x04, 0x08,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

class sfeDLGrWiFi : public sfeDLGraphic
{

  public:
    sfeDLGrWiFi() : _currentBitmap{3}
    {
        rect.width = kIconWiFiWidth + 2;
        rect.height = kIconWiFiHeight + 2;
    }

    void onDraw(flxDevMicroOLED *screen)
    {
        erase(screen);
        screen->bitmap(rect.x + 1, rect.y + 1, (uint8_t *)kIconsWiFi[_currentBitmap], kIconWiFiWidth, kIconWiFiHeight);
    }

    void updateValue(uint value)
    {
        if (value < 4 && _currentBitmap != value)
        {
            _currentBitmap = value;
            update();
        }
    }
    void listenForUpdate(flxSignalUInt &theEvent)
    {
        theEvent.call(this, &sfeDLGrWiFi::updateValue);
    }

  private:
    uint16_t _currentBitmap;
};
#endif

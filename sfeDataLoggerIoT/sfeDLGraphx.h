/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */
#pragma once

#ifdef THIS_IS_ENABLED
#include <cstdint>
#include <vector>

#include <Flux/flxCore.h>

// OLED
#include <Flux/flxDevMicroOLED.h>

struct grRect
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
};

class sfeDLGrView;

class sfeDLGraphic
{

  public:
    sfeDLGraphic() : rect{0, 0, 0, 0}, _needsUpdate{false}
    {
    }
    sfeDLGraphic(uint16_t x, uint16_t y)
    {
        rect.x = x;
        rect.y = y;
    }
    sfeDLGraphic(uint16_t x, uint16_t y, uint16_t width, uint16_t height) : sfeDLGraphic(x, y)
    {
        rect.width = width;
        rect.height = height;
    }

    grRect rect;

    void origin(uint16_t x, uint16_t y)
    {
        rect.x = x;
        rect.y = y;
    }

    void erase(flxDevMicroOLED *screen)
    {
        screen->rectangleFill(rect.x, rect.y, rect.width, rect.height, 0);
    }

    virtual void onDraw(flxDevMicroOLED *screen)
    {
    }
    void draw(flxDevMicroOLED *screen)
    {
        if (_needsUpdate)
        {
            onDraw(screen);
            _needsUpdate = false;
        }
    }

    void update(void)
    {
        _needsUpdate = true;
    }

  private:
    bool _needsUpdate;
};

class sfeDLGrView : public sfeDLGraphic
{

  public:
    void add(sfeDLGraphic *gr)
    {
        if (gr)
            _children.push_back(gr);
    }

    void draw(flxDevMicroOLED *screen)
    {
        for (sfeDLGraphic *pChild : _children)
            pChild->draw(screen);
        screen->display();
    }

    void update(flxDevMicroOLED *screen)
    {
        // do a full screen update
        screen->erase();

        // set update flags
        for (sfeDLGraphic *pChild : _children)
            pChild->update();

        draw(screen);
    }

  private:
    std::vector<sfeDLGraphic *> _children;
};

#endif
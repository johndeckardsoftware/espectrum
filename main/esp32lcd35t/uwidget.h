/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

// uwidget.h
#pragma once

#include <stdint.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class UIWidget {
public:
    virtual ~UIWidget() = default;
    virtual void draw() = 0;
    // Returns:
    //   >= 0 : confirmed choice / selected number
    //   -1   : cancelled / dismissed
    //   -2   : state changed, keep open (e.g. +/- stepped)
    virtual int16_t handleTouch(int16_t touchX, int16_t touchY) = 0;
    virtual const LGFX_Sprite* getSprite() const = 0;
};
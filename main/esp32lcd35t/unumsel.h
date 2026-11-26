/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

// unumsel.h
#pragma once

#include "uwidget.h"
#include "config.h"
#include <string>

class NumberSelector : public UIWidget {
public:
    NumberSelector(lgfx::LGFX_Device& display,
                   const std::string& label,
                   int32_t initialVal,
                   int32_t minVal,
                   int32_t maxVal,
                   int32_t step,
                   Config &config,
                   const std::string& key,
                   int16_t posX = 44,
                   int16_t posY = 66,
                   int16_t width = 320,
                   int16_t height = 180);

    void draw() override;
    int16_t handleTouch(int16_t touchX, int16_t touchY) override;
    const LGFX_Sprite* getSprite() const override { return &spr_; }
    int32_t value() const { return val_; }

private:
    lgfx::LGFX_Device& lcd_;
    LGFX_Sprite spr_;
    std::string label_;
    int32_t val_, min_, max_, step_;
    Config config_;
    std::string key_;
    int16_t posX_, posY_, width_, height_;
};
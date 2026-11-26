/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

// umsgbox.h
#pragma once

#include "uwidget.h"
#include <string>

class MessageBox : public UIWidget {
public:
    MessageBox(lgfx::LGFX_Device& display,
               const std::string& title,
               const std::string& message,
               bool twoButtons = false,
               int16_t posX = 44,
               int16_t posY = 66,
               int16_t width = 320,
               int16_t height = 180);

    void draw() override;
    int16_t handleTouch(int16_t touchX, int16_t touchY) override;
    const LGFX_Sprite* getSprite() const override { return &spr_; }

private:
    lgfx::LGFX_Device& lcd_;
    LGFX_Sprite spr_;
    std::string title_;
    std::string message_;
    bool twoButtons_;
    int16_t posX_, posY_, width_, height_;
};
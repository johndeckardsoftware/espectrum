/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

// umsgbox.cpp
#include "umsgbox.h"

MessageBox::MessageBox(lgfx::LGFX_Device& display,
                       const std::string& title,
                       const std::string& message,
                       bool twoButtons,
                       int16_t posX, int16_t posY, int16_t width, int16_t height)
    : lcd_(display), spr_(&lcd_), title_(title), message_(message),
      twoButtons_(twoButtons), posX_(posX), posY_(posY), width_(width), height_(height) {
    spr_.setColorDepth(16);
    spr_.setPsram(true);
    spr_.createSprite(width_, height_);
}

void MessageBox::draw() {
    spr_.fillSprite(TFT_NAVY);
    spr_.drawRect(0, 0, width_, height_, TFT_WHITE);
    spr_.drawRect(2, 2, width_ - 4, height_ - 4, TFT_LIGHTGREY);

    spr_.setTextSize(2);
    spr_.setTextColor(TFT_YELLOW, TFT_NAVY);
    spr_.setCursor(14, 14);
    spr_.print(title_.c_str());

    spr_.setTextColor(TFT_WHITE, TFT_NAVY);
    spr_.setCursor(14, 48);
    spr_.print(message_.c_str()); // Tip: wrap long strings or split on '\n'

    const int16_t btnY = height_ - 48;
    const int16_t btnH = 36;

    if (!twoButtons_) {
        const int16_t btnW = 100;
        const int16_t btnX = (width_ - btnW) / 2;
        spr_.fillRect(btnX, btnY, btnW, btnH, TFT_DARKGREY);
        spr_.drawRect(btnX, btnY, btnW, btnH, TFT_WHITE);
        spr_.setCursor(btnX + 34, btnY + 10);
        spr_.setTextColor(TFT_WHITE, TFT_DARKGREY);
        spr_.print("OK");
    } else {
        const int16_t btnW = 110;
        const int16_t leftX = 30;
        const int16_t rightX = width_ - 30 - btnW;

        spr_.fillRect(leftX, btnY, btnW, btnH, TFT_MAROON);
        spr_.drawRect(leftX, btnY, btnW, btnH, TFT_WHITE);
        spr_.setCursor(leftX + 18, btnY + 10);
        spr_.setTextColor(TFT_WHITE, TFT_MAROON);
        spr_.print("Cancel");

        spr_.fillRect(rightX, btnY, btnW, btnH, TFT_DARKGREEN);
        spr_.drawRect(rightX, btnY, btnW, btnH, TFT_WHITE);
        spr_.setCursor(rightX + 42, btnY + 10);
        spr_.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        spr_.print("OK");
    }

    spr_.pushSprite(posX_, posY_);
}

int16_t MessageBox::handleTouch(int16_t touchX, int16_t touchY) {
    const int16_t rx = touchX - posX_;
    const int16_t ry = touchY - posY_;
    if (rx < 0 || rx >= width_ || ry < 0 || ry >= height_) {
        return twoButtons_ ? -2 : -1;
    }

    const int16_t btnY = height_ - 48;
    const int16_t btnH = 36;
    if (ry < btnY || ry >= btnY + btnH) {
        return -2;
    }

    if (!twoButtons_) {
        const int16_t btnW = 100;
        const int16_t btnX = (width_ - btnW) / 2;
        if (rx >= btnX && rx < btnX + btnW) return 0;
        return -2;
    } else {
        const int16_t btnW = 110;
        const int16_t leftX = 30;
        const int16_t rightX = width_ - 30 - btnW;
        if (rx >= leftX && rx < leftX + btnW) return -1;
        if (rx >= rightX && rx < rightX + btnW) return 1;
        return -2;
    }
}
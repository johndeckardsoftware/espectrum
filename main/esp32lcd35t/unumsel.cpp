/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

// unumsel.cpp
#include "unumsel.h"
#include "config.h"
#include "../common/helper.h"
#include <algorithm>
#include <cstdio>

NumberSelector::NumberSelector(lgfx::LGFX_Device& display,
                               const std::string& label,
                               int32_t initialVal,
                               int32_t minVal,
                               int32_t maxVal,
                               int32_t step,
                               Config &config,
                               const std::string& key,
                               int16_t posX, int16_t posY, int16_t width, int16_t height)
    : lcd_(display), spr_(&lcd_), label_(label),
      val_(initialVal), min_(minVal), max_(maxVal), step_(step > 0 ? step : 1),
      config_(config), key_(key),
      posX_(posX), posY_(posY), width_(width), height_(height) {
    
    printf("1. key: %s, initialVal: %ld\n", key.c_str(), initialVal);
    Config::get(key, &initialVal);
    printf("2. key: %s, initialVal: %ld\n", key.c_str(), initialVal);

    val_ = initialVal;
    val_ = std::clamp(val_, min_, max_);
    spr_.setColorDepth(16);
    spr_.setPsram(true);
    spr_.createSprite(width_, height_);
}

void NumberSelector::draw() {
    spr_.fillSprite(TFT_DARKGREY);
    spr_.drawRect(0, 0, width_, height_, TFT_WHITE);

    spr_.setTextSize(2);
    spr_.setTextColor(TFT_WHITE, TFT_DARKGREY);
    spr_.setCursor(14, 14);
    spr_.print(label_.c_str());

    // '-' button, value box, '+' button at y = 56
    const int16_t midY = 56;
    spr_.fillRect(24, midY, 64, 44, TFT_NAVY);
    spr_.drawRect(24, midY, 64, 44, TFT_WHITE);
    spr_.setCursor(50, midY + 14);
    spr_.setTextColor(TFT_WHITE, TFT_NAVY);
    spr_.print("-");

    spr_.fillRect(98, midY, 124, 44, TFT_BLACK);
    spr_.drawRect(98, midY, 124, 44, TFT_LIGHTGREY);
    char buf[24];
    snprintf(buf, sizeof(buf), "%ld", static_cast<long>(val_));
    spr_.setTextColor(TFT_CYAN, TFT_BLACK);
    spr_.setCursor(116, midY + 14);
    spr_.print(buf);

    spr_.fillRect(232, midY, 64, 44, TFT_NAVY);
    spr_.drawRect(232, midY, 64, 44, TFT_WHITE);
    spr_.setTextColor(TFT_WHITE, TFT_NAVY);
    spr_.setCursor(258, midY + 14);
    spr_.print("+");

    // Cancel / OK at bottom
    const int16_t botY = height_ - 48;
    spr_.fillRect(24, botY, 120, 36, TFT_MAROON);
    spr_.drawRect(24, botY, 120, 36, TFT_WHITE);
    spr_.setCursor(44, botY + 10);
    spr_.setTextColor(TFT_WHITE, TFT_MAROON);
    spr_.print("Cancel");

    spr_.fillRect(176, botY, 120, 36, TFT_DARKGREEN);
    spr_.drawRect(176, botY, 120, 36, TFT_WHITE);
    spr_.setCursor(220, botY + 10);
    spr_.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    spr_.print("OK");

    spr_.pushSprite(posX_, posY_);
}

int16_t NumberSelector::handleTouch(int16_t touchX, int16_t touchY) {
    const int16_t rx = touchX - posX_;
    const int16_t ry = touchY - posY_;
    if (rx < 0 || rx >= width_ || ry < 0 || ry >= height_) return -2;

    const int16_t midY = 56;
    if (ry >= midY && ry < midY + 44) {
        if (rx >= 24 && rx < 88) {
            val_ = std::max(min_, val_ - step_);
            draw();
            return -2;
        }
        if (rx >= 232 && rx < 296) {
            val_ = std::min(max_, val_ + step_);
            draw();
            return -2;
        }
    }

    const int16_t botY = height_ - 48;
    if (ry >= botY && ry < botY + 36) {
        if (rx >= 24 && rx < 144) return -1;
        if (rx >= 176 && rx < 296) {
            config_.set(key_, val_);
            printf("3. key_: %s, val_: %ld\n", key_.c_str(), val_);
            return static_cast<int16_t>(val_);
        }
    }

    return -2;
}
/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include "display.h"
#include <algorithm>
#include <chrono>

namespace
{
Color colorFrom565(uint16_t color)
{
    color = static_cast<uint16_t>((color << 8) | (color >> 8));
    return Color{
        static_cast<unsigned char>(((color >> 11) & 0x1F) * 255 / 31),
        static_cast<unsigned char>(((color >> 5) & 0x3F) * 255 / 63),
        static_cast<unsigned char>((color & 0x1F) * 255 / 31),
        255
    };
}
}

DisplaySystem::DisplaySystem()
    : framebuffer(FRAMEBUFFER_SIZE)
{
}

DisplaySystem::~DisplaySystem()
{
    if (texture.id != 0)
        UnloadTexture(texture);
}

void DisplaySystem::init()
{
    Image image = GenImageColor(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, BLACK);
    texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
}

void DisplaySystem::renderSpectrumScreen(ZXSpectrum *spectrum)
{
    const auto start = std::chrono::steady_clock::now();
    const uint8_t *video = spectrum->mem.getScreenData();
    const uint16_t *palette = spectrum->palette_565;
    const Color border = colorFrom565(palette[spectrum->port.borderColor & 0x0F]);
    const bool blink = (spectrum->port.blink_counter & 0x10) != 0;

    for (int y = 0; y < FRAMEBUFFER_HEIGHT; ++y)
    {
        Color *row = framebuffer.data() + y * FRAMEBUFFER_WIDTH;
        if (y < BORDER_TOP || y >= FRAMEBUFFER_HEIGHT - BORDER_BOTTOM)
        {
            std::fill(row, row + FRAMEBUFFER_WIDTH, border);
            continue;
        }

        const uint16_t yy = static_cast<uint16_t>(y - BORDER_TOP);
        const uint16_t yOffset = ((yy & 0xC0) << 5) | ((yy & 0x07) << 8) | ((yy & 0x38) << 2);
        std::fill(row, row + BORDER_LEFT, border);
        Color *pixel = row + BORDER_LEFT;

        for (uint16_t x = 0; x < 32; ++x)
        {
            const uint8_t pixels = video[yOffset | x];
            const uint8_t attr = video[0x1800 + (((yy & ~0x7) << 2) | x)];
            uint8_t foreground = (attr & 0x80) && blink ? (attr >> 3) & 7 : attr & 7;
            uint8_t background = (attr & 0x80) && blink ? attr & 7 : (attr >> 3) & 7;
            foreground |= (attr & 0x40) >> 3;
            background |= (attr & 0x40) >> 3;
            const Color fg = colorFrom565(palette[foreground]);
            const Color bg = colorFrom565(palette[background]);

            for (int bit = 7; bit >= 0; --bit)
                *pixel++ = (pixels & (1 << bit)) ? fg : bg;
        }

        std::fill(pixel, row + FRAMEBUFFER_WIDTH, border);
    }

    ++spectrum->port.blink_counter;
    UpdateTexture(texture, framebuffer.data());
    renderTimeUs = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count());
}

void DisplaySystem::draw() const
{
    const float scale = std::min(
        static_cast<float>(GetScreenWidth()) / FRAMEBUFFER_WIDTH,
        static_cast<float>(GetScreenHeight()) / FRAMEBUFFER_HEIGHT);
    const float width = FRAMEBUFFER_WIDTH * scale;
    const float height = FRAMEBUFFER_HEIGHT * scale;
    const Rectangle source{0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)};
    const Rectangle destination{
        (GetScreenWidth() - width) * 0.5f,
        (GetScreenHeight() - height) * 0.5f,
        width,
        height
    };
    DrawTexturePro(texture, source, destination, Vector2{0, 0}, 0.0f, WHITE);
}

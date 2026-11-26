/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#ifndef DISPLAY_SYSTEM_H
#define DISPLAY_SYSTEM_H

#include <cstdint>
#include <vector>
#include <raylib.h>
#include "../spectrum/zx48k.h"

#define BORDER_TOP         16
#define BORDER_LEFT        16
#define BORDER_RIGHT       16
#define BORDER_BOTTOM      16
#define SCREEN_WIDTH       256
#define SCREEN_HEIGHT      192
#define FRAMEBUFFER_WIDTH  (BORDER_LEFT + SCREEN_WIDTH + BORDER_RIGHT)
#define FRAMEBUFFER_HEIGHT (BORDER_TOP + SCREEN_HEIGHT + BORDER_BOTTOM)
#define FRAMEBUFFER_SIZE   (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT)

class DisplaySystem
{
private:
    Texture2D texture{};
    std::vector<Color> framebuffer;
    uint32_t renderTimeUs = 0;

public:
    DisplaySystem();
    ~DisplaySystem();

    void init();
    void renderSpectrumScreen(ZXSpectrum *spectrum);
    void draw() const;

    inline uint32_t getLcdRenderTimeUs() const { return renderTimeUs; }
};

#endif // DISPLAY_SYSTEM_H


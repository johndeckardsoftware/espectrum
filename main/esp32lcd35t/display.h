/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#ifndef DISPLAY_SYSTEM_H
#define DISPLAY_SYSTEM_H

#include <stdint.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "spectrum/zx48k.h"
#include "virtual_keyboard.h"

// Hardware screen dimensions
#define LCD_WIDTH          320
#define LCD_HEIGHT         480
#define LCD_ROTATION       270

// ZX Spectrum screen area and borders structural offsets
#define BORDER_TOP         16
#define BORDER_LEFT        16
#define BORDER_RIGHT       16
#define BORDER_BOTTOM      16
#define SCREEN_WIDTH       256
#define SCREEN_HEIGHT      192

#define FRAMEBUFFER_WIDTH  (BORDER_LEFT + SCREEN_WIDTH + BORDER_RIGHT)   // 288 px
#define FRAMEBUFFER_HEIGHT (BORDER_TOP + SCREEN_HEIGHT + BORDER_BOTTOM)  // 224 px
#define FRAMEBUFFER_SIZE   (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT)

/**
 * @brief Customized LovyanGFX device driver configuration for Waveshare ST7796 display.
 */
class LGFX_Device_Instance : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7796 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;
    lgfx::Light_PWM    _light_instance;

public:
    LGFX_Device_Instance();
};

/**
 * @brief Subsystem managing the Sinclair VRAM decoding, DMA framebuffers, and display rendering.
 */
class DisplaySystem
{
private:
    LGFX_Device_Instance lcd;       ///< Hardware LovyanGFX instance.
    uint16_t *zxFrameBuffer;        ///< Pointer to memory block allocated for DMA transfers.
    
    // Internal timing telemetry metrics
    uint32_t lcdRenderTimeUs;
    uint32_t lcdPushTimeUs;

public:
    DisplaySystem();
    ~DisplaySystem();

    /**
     * @brief Initializes LovyanGFX panel configs and secures the DMA memory blocks.
     */
    void init();

    /**
     * @brief Fetches the underlying LovyanGFX device reference (needed for sprites blending).
     */
    LGFX_Device_Instance& getLcdDriver();

    /**
     * @brief Translates Sinclair non-linear video memory cells into standard RGB565 rows.
     */
    void renderSpectrumScreen(ZXSpectrum *spectrum);

    /**
     * @brief Flushes the updated framebuffer asynchronously out to the display over SPI DMA channels.
     */
    void pushFrameDMA();

    /**
     * @brief Adjusts the hardware panel backlight level.
     */
    void setBacklight(uint8_t brightness);

    void switchOffSpectrumScreen(void);
    esp_err_t saveSpectrumScreenshot(const char *filepath);
    esp_err_t saveFullScreenshot(const char *filepath, const VirtualKeyboard *vkb, const LGFX_Sprite *overlay = nullptr);
    
    // Telemetry getters
    inline uint32_t getLcdRenderTimeUs() const { return lcdRenderTimeUs; }
    inline uint32_t getLcdPushTimeUs() const { return lcdPushTimeUs; }
};

#endif // DISPLAY_SYSTEM_H


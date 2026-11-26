/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include "display.h"
#include "config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_3inch5_lcd_port.h"
#include "virtual_keyboard.h"

static const char *TAG = "display";

LGFX_Device_Instance::LGFX_Device_Instance()
{
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 80000000; // 80 MHz SPI bus write speed
        cfg.freq_read = 16000000;
        cfg.spi_3wire = false;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = 5; 
        cfg.pin_mosi = 1; 
        cfg.pin_miso = 2; 
        cfg.pin_dc = 3;   
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }
    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs = -1;
        cfg.pin_rst = -1; // Reset physically anchored over expanders lanes
        cfg.pin_busy = -1;
        cfg.memory_width = LCD_WIDTH;
        cfg.memory_height = LCD_HEIGHT;
        cfg.panel_width = LCD_WIDTH;
        cfg.panel_height = LCD_HEIGHT;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
//        cfg.readable = false;
        cfg.readable = true;
        cfg.invert = true;
        cfg.rgb_order = false;
        cfg.dlen_16bit = false;
//        cfg.bus_shared = false;
        cfg.bus_shared = true;
        _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
}

DisplaySystem::DisplaySystem()
    : zxFrameBuffer(nullptr), lcdRenderTimeUs(0), lcdPushTimeUs(0)
{
}

DisplaySystem::~DisplaySystem()
{
    if (zxFrameBuffer != nullptr) {
        heap_caps_free(zxFrameBuffer);
    }
}

void DisplaySystem::init()
{
    lcd.init();
    lcd.setRotation(1);
    lcd.fillScreen(TFT_BLACK);
    int32_t backlit = 60;
    Config::get("brightness", &backlit);
    setBacklight(backlit);
    printf("Backlight: %ld\n", backlit);

    size_t bufSize = FRAMEBUFFER_SIZE * sizeof(uint16_t);
    // Secure high speed internal RAM regions first to optimize DMA engines performance
    zxFrameBuffer = (uint16_t *)heap_caps_malloc(bufSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    
    if (zxFrameBuffer == nullptr) {
        ESP_LOGW(TAG, "Internal DMA memory exhaustion. Falling back to SPIRAM region block.");
        zxFrameBuffer = (uint16_t *)heap_caps_malloc(bufSize, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    }

    if (zxFrameBuffer == nullptr) {
        ESP_LOGE(TAG, "FATAL: Graphics frame buffers allocation failure. Halting.");
        abort();
    }
}

LGFX_Device_Instance& DisplaySystem::getLcdDriver()
{
    return lcd;
}

void DisplaySystem::setBacklight(uint8_t brightness)
{
    esp_3inch5_brightness_port_set(brightness);
}

void DisplaySystem::renderSpectrumScreen(ZXSpectrum *spectrum)
{
    uint64_t startUs = esp_timer_get_time();
    
    const uint8_t *vidmem = spectrum->mem.getScreenData();
    const uint16_t *palette = spectrum->palette_565;
    
    uint16_t brdColor = palette[spectrum->port.borderColor & 0x0F];
    const bool blinkPhase = 0 != (spectrum->port.blink_counter & 0x10);
    uint16_t fgc = 0, bgc = 0;

    for (int y = 0; y < FRAMEBUFFER_HEIGHT; y++) {
        uint16_t *dst = zxFrameBuffer + (y * FRAMEBUFFER_WIDTH);
        
        if ((y < BORDER_TOP) || (y >= (FRAMEBUFFER_HEIGHT - BORDER_BOTTOM))) {
            for (int x = 0; x < FRAMEBUFFER_WIDTH; x++) {
                *dst++ = brdColor;
            }
        } 
        else {
            const uint16_t yy = y - BORDER_TOP;
            // Unroll Sinclair interleaved display coordinate matrix offsets formulas
            const uint16_t yOffset = ((yy & 0xC0) << 5) | ((yy & 0x07) << 8) | ((yy & 0x38) << 2);

            for (int x = 0; x < BORDER_LEFT; x++) {
                *dst++ = brdColor;
            }

            for (uint16_t x = 0; x < 32; x++) {
                const uint16_t pixOffset = yOffset | x;
                const uint16_t clrOffset = 0x1800 + (((yy & ~0x7) << 2) | x);

                const uint8_t pixByte = vidmem[pixOffset];
                const uint8_t clrByte = vidmem[clrOffset];

                uint8_t fg, bg;
                if ((clrByte & (1 << 7)) && blinkPhase) {
                    fg = (clrByte >> 3) & 7;
                    bg = clrByte & 7;
                } else {
                    fg = clrByte & 7;
                    bg = (clrByte >> 3) & 7;
                }
                
                fg |= (clrByte & (1 << 6)) >> 3;
                bg |= (clrByte & (1 << 6)) >> 3;
                
                fgc = palette[fg];
                bgc = palette[bg];

                for (int px = 7; px >= 0; px--) {
                    *dst++ = (pixByte & (1 << px)) ? fgc : bgc;
                }
            }

            for (int x = 0; x < BORDER_RIGHT; x++) {
                *dst++ = brdColor;
            }
        }
    }

    spectrum->port.blink_counter++;
    lcdRenderTimeUs = esp_timer_get_time() - startUs;
}

void DisplaySystem::pushFrameDMA()
{
    uint64_t startUs = esp_timer_get_time();
    // Hardware transfer over background DMA engine channels
    lcd.pushImageDMA(96, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, zxFrameBuffer);
    lcdPushTimeUs = esp_timer_get_time() - startUs;
}

void DisplaySystem::switchOffSpectrumScreen()
{
    lcd.fillRect(96, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, TFT_BLACK);
}

esp_err_t DisplaySystem::saveFullScreenshot(const char *filepath, const VirtualKeyboard *vkb, const LGFX_Sprite *overlay)
{
    if (!zxFrameBuffer || !vkb) {
        return ESP_ERR_INVALID_STATE;
    }

    const uint32_t W = 480;
    const uint32_t H = 320;
    const uint32_t img_size  = W * H * 2;
    const uint32_t file_size = 66 + img_size;
    const int32_t  neg_h     = -(int32_t)H; // Top-down row order

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing", filepath);
        return ESP_FAIL;
    }

    // 66-byte 16-bit RGB565 BMP Header
    uint8_t header[66] = {
        'B', 'M',
        (uint8_t)(file_size), (uint8_t)(file_size >> 8), (uint8_t)(file_size >> 16), (uint8_t)(file_size >> 24),
        0, 0, 0, 0, 66, 0, 0, 0, 40, 0, 0, 0,
        (uint8_t)(W), (uint8_t)(W >> 8), (uint8_t)(W >> 16), (uint8_t)(W >> 24),
        (uint8_t)(neg_h), (uint8_t)(neg_h >> 8), (uint8_t)(neg_h >> 16), (uint8_t)(neg_h >> 24),
        1, 0, 16, 0, 3, 0, 0, 0,
        (uint8_t)(img_size), (uint8_t)(img_size >> 8), (uint8_t)(img_size >> 16), (uint8_t)(img_size >> 24),
        0x13, 0x0B, 0, 0, 0x13, 0x0B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x00, 0xF8, 0x00, 0x00, // Red mask:   0xF800
        0xE0, 0x07, 0x00, 0x00, // Green mask: 0x07E0
        0x1F, 0x00, 0x00, 0x00  // Blue mask:  0x001F
    };
    fwrite(header, 1, sizeof(header), f);

    const uint16_t *leftBuf   = vkb->getLeftBuffer();
    const uint16_t *rightBuf  = vkb->getRightBuffer();
    const uint16_t *bottomBuf = vkb->getBottomBuffer();

    // Optional menu overlay buffer (pushed at x=10, y=10)
    const uint16_t *ovlBuf = overlay ? (const uint16_t *)overlay->getBuffer() : nullptr;
    const int32_t   ovlX   = 10;
    const int32_t   ovlY   = 10;
    const int32_t   ovlW   = overlay ? overlay->width()  : 0;
    const int32_t   ovlH   = overlay ? overlay->height() : 0;

    uint16_t row_buf[480];

    for (uint32_t y = 0; y < H; y++) {
        // 1. Compose base layer (VirtualKeyboard + ZX Spectrum screen)
        if (y < 224) {
            const uint16_t *srcL = leftBuf       + (y * 96);
            const uint16_t *srcC = zxFrameBuffer + (y * 288);
            const uint16_t *srcR = rightBuf      + (y * 96);

            for (uint32_t x = 0; x < 96; x++)  row_buf[x]       = __builtin_bswap16(srcL[x]);
            for (uint32_t x = 0; x < 288; x++) row_buf[96 + x]  = __builtin_bswap16(srcC[x]);
            for (uint32_t x = 0; x < 96; x++)  row_buf[384 + x] = __builtin_bswap16(srcR[x]);
        } else {
            const uint16_t *srcB = bottomBuf + ((y - 224) * 480);
            for (uint32_t x = 0; x < 480; x++) row_buf[x] = __builtin_bswap16(srcB[x]);
        }

        // 2. Blend/overwrite active menu sprite if present on this scanline
        if (ovlBuf && (int32_t)y >= ovlY && (int32_t)y < (ovlY + ovlH)) {
            const uint16_t *srcOvl = ovlBuf + (((int32_t)y - ovlY) * ovlW);
            int32_t maxX = (ovlX + ovlW > (int32_t)W) ? ((int32_t)W - ovlX) : ovlW;
            for (int32_t x = 0; x < maxX; x++) {
                row_buf[ovlX + x] = __builtin_bswap16(srcOvl[x]);
            }
        }

        fwrite(row_buf, sizeof(uint16_t), W, f);
    }

    fclose(f);
    ESP_LOGI(TAG, "Full screenshot saved to %s", filepath);
    return ESP_OK;
}

esp_err_t DisplaySystem::saveSpectrumScreenshot(const char *filepath) {
    if (!zxFrameBuffer) return ESP_ERR_INVALID_STATE;

    const uint32_t w = FRAMEBUFFER_WIDTH;   // 288
    const uint32_t h = FRAMEBUFFER_HEIGHT;  // 224
    const uint32_t img_size = w * h * 2;
    const uint32_t file_size = 66 + img_size;
    const int32_t neg_h = -(int32_t)h;      // Top-down row order

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", filepath);
        return ESP_FAIL;
    }

    // 66-byte 16-bit RGB565 BMP Header
    uint8_t header[66] = {
        'B', 'M',
        (uint8_t)(file_size), (uint8_t)(file_size >> 8), (uint8_t)(file_size >> 16), (uint8_t)(file_size >> 24),
        0, 0, 0, 0, 66, 0, 0, 0, 40, 0, 0, 0,
        (uint8_t)(w), (uint8_t)(w >> 8), (uint8_t)(w >> 16), (uint8_t)(w >> 24),
        (uint8_t)(neg_h), (uint8_t)(neg_h >> 8), (uint8_t)(neg_h >> 16), (uint8_t)(neg_h >> 24),
        1, 0, 16, 0, 3, 0, 0, 0,
        (uint8_t)(img_size), (uint8_t)(img_size >> 8), (uint8_t)(img_size >> 16), (uint8_t)(img_size >> 24),
        0x13, 0x0B, 0, 0, 0x13, 0x0B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x00, 0xF8, 0x00, 0x00, // Red mask
        0xE0, 0x07, 0x00, 0x00, // Green mask
        0x1F, 0x00, 0x00, 0x00  // Blue mask
    };
    fwrite(header, 1, sizeof(header), f);

    // Write row-by-row, un-swapping big-endian RGB565 bytes for standard BMP format
    uint16_t line_buf[FRAMEBUFFER_WIDTH];
    for (uint32_t y = 0; y < h; y++) {
        const uint16_t *src = zxFrameBuffer + (y * w);
        for (uint32_t x = 0; x < w; x++) {
            line_buf[x] = __builtin_bswap16(src[x]); // Remove __builtin_bswap16 if colors are inverted
        }
        fwrite(line_buf, sizeof(uint16_t), w, f);
    }

    fclose(f);
    ESP_LOGI(TAG, "Saved Spectrum screenshot to %s", filepath);
    return ESP_OK;
}

/*
 * Original Author: atomic14
 * Original Source: https://github.com/atomic14/esp32-zxspectrum
 * Original License: GNU General Public License v3.0
 *
 * Adapted by:      johndeckardsoftware
 * Adaptation Date: November 2026
 * Modifications:   Adapted for ESP32-S3-Touch-LCD-3.5_Rev2.0 board using ESP32/ESP-IDF platform.
 *
 */

#ifndef ZX48K_H
#define ZX48K_H

#include <stdint.h>
#include "../common/helper.h"
#ifdef ESPECTRUM_DESKTOP
#include "ubuntu/config.h"
#else
#include "esp32lcd35t/config.h"
#endif
#include "z80.hpp"
#include "memory.h"
#include "port.h"
#include "keyboard.h"
#include "keyboard_defs.h"
#include "tape_manager.h"
#include "AySound.h"

#define KEY_PRESSED     1
#define KEY_RELEASED    0


/**
 * @brief Supported ZX Spectrum hardware models.
 */
#include "zx_types.h"


/**
 * @brief Main ZX Spectrum emulator core class.
 */
class ZXSpectrum
{
public:
    int type = ZX_MODEL;
    Memory mem;                     ///< Memory management unit (ROM/RAM).
    Z80 z80;                        ///< Z80 chip.
    Keyboard keyboard;              ///< Keyboard handler.
    AySound ay;                     ///< AY8912 chip
    Port port{mem, keyboard, ay};   ///< Input/Output port
    TapeManager tm;                 ///< Tape Manager
    Config config;

    uint16_t soundSample = 0;       ///< Sound sample per line frame (beeper + AY)

    bool flashLoader = true;
    int _lastReadTapeIndex = 0;

    /// RGB565 16-color palette (Normal and Bright variants).
    const uint16_t palette_565[16] = {
        0x0000, 0x1B00, 0x00B8, 0x17B8, 0xE005, 0xF705, 0xE0BD, 0x18C6,
        0x0000, 0x1F00, 0x00F8, 0x1FF8, 0xE007, 0xFF07, 0xE0FF, 0xFFFF
    };

    bool paused = false;           ///< Emulation pause state flag.

    // Constructor & Lifecycle
    ZXSpectrum();

    bool initZX48K();

    // Core Emulation Controls
    int runForFrame(uint16_t *accumOut);

    // CPU Execution & Sound Tracking
    inline int runForCycles(int cycles)
    {
        int used = z80.execute(cycles);

        // Accumulate t-states if the beeper is currently active
        //if (port.earBit || port.micOutBit)
        if (port.earBit)
        {
            soundSample += used;
        }
        return used;
    }

    // Input Handling
    void updateKey(SpecKeys key, uint8_t state);
    void pressKey(SpecKeys key);
    void sendKeys(const char *text);

    // Tape handling
    void tapePlaying(uint16_t pc, int op_tstates);
    void tapeLoaded(void);
    void trapTapeLoad(void);

    inline void setMicHigh() { port.micLevel = true; }
    inline void setMicLow()  { port.micLevel = false; }
    inline void toggleMic()  { port.micLevel = !port.micLevel; }
};

#endif // ZX48K_H

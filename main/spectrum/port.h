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
#pragma once
#include <cstdint>
#include "memory.h"
#include "keyboard.h"
#include "AySound.h"

class Port
{
private:
    Memory& mem;
    Keyboard& keyboard;
    AySound& ay;

public:
    // Constructor takes a Keyboard reference and binds it immediately
    Port(Memory _mem, Keyboard& kb, AySound& _ay) : mem(_mem), keyboard(kb), ay(_ay) {}

    uint8_t borderColor = 7;       ///< Current active border color (0-7).
    uint8_t blink_counter = 0;     ///< Counter used for flashing attributes.

    uint8_t earBit = 0;             ///< Bit 4 from port 0xFE (beeper active state).
    uint8_t micOutBit = 0;          ///< Bit 3 from port 0xFE (mic output feed back).
    uint8_t micInBit = 0;           ///< Bit 5 from port 0xFE (mic input).
    uint8_t micLevel = 0;           ///< Current MIC/EAR port state.

    bool emulate_FF = false;        ///< Floating bus emulation flag.
    uint8_t portFF = 0xff;          ///< Last read byte from floating bus.

    bool ayUsed = false;           ///< true for (port & 0x8002) == 0x8000
    uint8_t lastAyRegister = 0;
    uint16_t lastPagingValue = 0;

    // I/O Port Emulation
    inline uint8_t in(uint16_t port)
    {
        // Handle ULA Port 0xFE (Keyboard, Border, Mic)
        if ((port & 0x01) == 0)
        {
            // Read the 5-bit keyboard matrix using the high byte of the port address (A8-A15)
            uint8_t data = keyboard.flushAndReadMatrix(port >> 8);

            // Add MIC Input Bit 5
            if (micInBit != 0)
                micInBit = micInBit;
            data |= micInBit;

            return data;
        }

        // Floating bus Port 0xFF emulation
        if ((port & 0xFF) == 0xFF && emulate_FF)
        {
            return portFF;
        }
        return 0xFF;
    }

    inline void out(uint16_t port, uint8_t data)
    {
        // Handle ULA Port 0xFE (Border color, Beeper and Mic)
        if (!(port & 0x01))
        {
            borderColor = (data & 0x07);        // Bits 0-2: Border Color
            earBit =    (data & 0b00010000);    // Bit 4: Beeper activation
            micOutBit = (data & 0b00001000);    // Bit 3: Ouput Mic (feed back of tape load)
        }
        else
        {
            if ((port & 0x8002) == 0x8000) // AY chip
            {
                ayUsed = true;
                if ((port & 0x4000) != 0)
                {
                    lastAyRegister = data;
                    ay.selectRegister(data);
                }
                else
                {
                    ay.setRegisterData(data);
                }
            }
            else if (!(port & 0x8002))  //  128/+2 paging
            {
                mem.write_7ffd(data);
            }
        }
    }
};
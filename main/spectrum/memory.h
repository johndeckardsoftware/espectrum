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
#include <cstring>

#include "common/helper.h"
#include "zx_types.h"

class Memory
{
private:
    static constexpr uint16_t page_size = 0x4000;

#if ZX_MODEL == ZX48K
    static constexpr int ram_bank_count = 3;
    static constexpr int physical_page_count = 4;
    static constexpr int rom_bank_count = 1;
    static constexpr int rom_page_offset = 0;
#else
    static constexpr int ram_bank_count = 8;
    static constexpr int physical_page_count = 10;
    static constexpr int rom_bank_count = 2;
    static constexpr int rom_page_offset = 8;
#endif

    int type;
    uint8_t physical_pages[physical_page_count][page_size];
    uint8_t* read_pages[4];
    uint8_t* write_pages[4];
    uint8_t port_7ffd_state = 0;
    bool paging_locked = true;

    uint8_t* getRamBank(int bank)
    {
#if ZX_MODEL == ZX48K
        if (bank < 1 || bank > ram_bank_count) {
            return nullptr;
        }
        return physical_pages[bank];
#else
        if (bank < 0 || bank >= ram_bank_count) {
            return nullptr;
        }
        return physical_pages[bank];
#endif
    }

public:
    explicit Memory(int _type) : type(_type)
    {
        std::memset(physical_pages, 0, sizeof(physical_pages));

#if ZX_MODEL == ZX48K
        read_pages[0] = physical_pages[0];
        read_pages[1] = physical_pages[1];
        read_pages[2] = physical_pages[2];
        read_pages[3] = physical_pages[3];
#else
        read_pages[0] = physical_pages[rom_page_offset];
        read_pages[1] = physical_pages[5];
        read_pages[2] = physical_pages[2];
        read_pages[3] = physical_pages[0];
#endif

        write_pages[0] = nullptr;
        write_pages[1] = read_pages[1];
        write_pages[2] = read_pages[2];
        write_pages[3] = read_pages[3];

        _print("mmu", "Memory Manager Initialized for %dK model", type);
    }

    void write_7ffd(uint8_t value)
    {
#if ZX_MODEL == ZX128K
        if (type != ZX128K || (port_7ffd_state & 0x20)) {
            return;
        }
        
        if (paging_locked) return;

        port_7ffd_state = value;

        // Bits 0-2: RAM page (0-7) to map into memory at 0xc000.
        const uint8_t ram_page = value & 0x07;
        read_pages[3] = physical_pages[ram_page];
        write_pages[3] = physical_pages[ram_page];

        // Bit 3: Select normal (0) or shadow (1) screen to be displayed.
        // The normal screen is in bank 5, whilst the shadow screen is in bank 7.
        // Note that this does not affect the memory between 0x4000 and 0x7fff, which is always bank 5.
        const uint8_t screen_bank = (value & 0x08) ? 7 : 5;
        read_pages[1] = physical_pages[screen_bank];
        write_pages[1] = physical_pages[screen_bank];

        // Bit 4: ROM select. ROM 0 is the 128k editor and menu system; ROM 1 contains 48K BASIC.
        const uint8_t rom_bank = (value & 0x10) ? 1 : 0;
        read_pages[0] = physical_pages[rom_page_offset + rom_bank];

        // Bit 5: If set, memory paging will be disabled and further output to this port will be ignored until the computer is reset.
        paging_locked = value & 0x20
#else
        (void)value;
#endif
    }

    inline uint8_t read(uint16_t address) const
    {
        const uint16_t slot = address >> 14;
        return read_pages[slot][address & 0x3FFF];
    }

    inline void write(uint16_t address, uint8_t value)
    {
        const uint16_t slot = address >> 14;
        if (write_pages[slot] != nullptr) {
            write_pages[slot][address & 0x3FFF] = value;
        }
    }

    inline void loadRamPage(uint8_t bank, const uint8_t* data)
    {
        uint8_t* destination = getRamBank(bank);
        if (destination != nullptr && data != nullptr) {
            std::memcpy(destination, data, page_size);
        }
    }

    void loadRom(int bank_index, const uint8_t* rom_data, int rom_len)
    {
        if (bank_index < 0 || bank_index >= rom_bank_count ||
            rom_data == nullptr || rom_len <= 0) {
            return;
        }

        if (rom_len > page_size) {
            rom_len = page_size;
        }

        std::memcpy(physical_pages[rom_page_offset + bank_index], rom_data,
                    static_cast<size_t>(rom_len));
        _print("mmu", "ROM Bank %d loaded: %d bytes", bank_index, rom_len);
    }

    void loadRom(const uint8_t* rom_data, int rom_len)
    {
        loadRom(0, rom_data, rom_len);
    }

    inline uint16_t getPageSize() const
    {
        return page_size;
    }

    inline uint8_t* getScreenData()
    {
        return read_pages[1];
    }

    inline uint8_t* getBankRam(int bank)
    {
        return getRamBank(bank);
    }

    inline uint8_t* getRamBankData(int bank)
    {
        return getRamBank(bank);
    }

    inline uint8_t* getFlatRam()
    {
#if ZX_MODEL == ZX48K
        return getRamBank(1);
#else
        return getRamBank(0);
#endif
    }

    void dumpMemory() {
        uint16_t addr = 0;
        for (int i = 0; i < physical_page_count; i++) {
            for (int j = 0; j < 512; j++) {
                printf("%04x: ", addr);
                for (int k = 0; k < 32; k++) {
                    printf("%02x ", read(addr++));
                }
                printf("\n");
                fflush(stdout);
            }
        }
    }
};

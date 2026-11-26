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
#include <stdlib.h>
#include <string.h>
#include "../common/helper.h"

// ZX Spectrum 48K-128K Memory Manager with 16KB Page Tables
class Memory
{
private:
    // 128K Physical memory banks
    //   2 ROM banks (16K each) = 32KB
    //   8 RAM banks (16K each) = 128KB
    // 48K Physical memory banks
    //   1 ROM banks (16K each) = 16KB
    //   3 RAM banks (16K each) = 48KB
    //static const uint8_t rom_pages = 1;
    static const uint8_t ram_pages = 4;
    static const uint16_t page_size = 0x4000;
    uint8_t ram_banks[ram_pages][0x4000];

    // Dummy write page to swallow writes to ROM without crashing
    uint8_t dummy_write_page[0x4000];

    // 16KB Pointers for the 4 address slots:
    // Slot 0: 0x0000 - 0x3FFF (ROM / Paged RAM)
    // Slot 1: 0x4000 - 0x7FFF (RAM Bank 5 - Always Screen VRAM in 128K)
    // Slot 2: 0x8000 - 0xBFFF (RAM Bank 2)
    // Slot 3: 0xC000 - 0xFFFF (Switched RAM Bank 0/1/3/4/6/7)
    uint8_t* read_pages[4];
    uint8_t* write_pages[4];

    // 128K Paging State Registers (Port 0x7FFD)
    uint8_t port_7ffd_state = 0;

public:
    Memory()
    {
        // Clear all physical banks
        memset(ram_banks, 0, sizeof(ram_banks));
        memset(dummy_write_page, 0, sizeof(dummy_write_page));

        // Flat 48K pages layout setup upon boot:
        // Slot 0 (0x0000): ROM 0
        // Slot 1 (0x4000): RAM Bank 0
        // Slot 2 (0x8000): RAM Bank 1
        // Slot 3 (0xC000): RAM Bank 2
        read_pages[0] = ram_banks[0];
        write_pages[0] = dummy_write_page; // Write protect ROM

        read_pages[1] = ram_banks[1];
        write_pages[1] = ram_banks[1];

        read_pages[2] = ram_banks[2];
        write_pages[2] = ram_banks[2];

        read_pages[3] = ram_banks[3];
        write_pages[3] = ram_banks[3];

        _print("mmu", "Memory Manager Initialized");
    }

    ~Memory() = default;

    // --- Paging Control (Port 0x7FFD) ---
    void write_7ffd(uint8_t value)
    {
        // If bit 5 is set, paging is locked (disabled until reset)
        if (port_7ffd_state & 0x20) {
            return;
        }

        port_7ffd_state = value;

        // Bit 0-2: Selects RAM page (0-7) mapped to bank 3 (0xC000 - 0xFFFF)
        uint8_t ram_page = value & 0x07;
        read_pages[3] = ram_banks[ram_page];
        write_pages[3] = ram_banks[ram_page];

        // Bit 3: Screen select (0 = Bank 5 @ 0x4000, 1 = Bank 7 @ 0x4000)
        uint8_t screen_bank = (value & 0x08) ? 7 : 5;
        read_pages[1] = ram_banks[screen_bank];
        write_pages[1] = ram_banks[screen_bank];

        // Bit 4: ROM select (0 = 128K Editor ROM, 1 = 48K BASIC ROM)
        uint8_t rom_bank = (value & 0x10) ? 1 : 0;
        read_pages[0] = ram_banks[rom_bank];

        // Note: Bit 5 locks further 7FFD writes when set to 1
    }

    inline uint8_t read(uint16_t address)
    {
        // High 2 bits select the 16KB slot (0 to 3), lower 14 bits are the offset
        return read_pages[address >> 14][address & 0x3FFF];
    }

    inline void write(uint16_t address, uint8_t value)
    {
        write_pages[address >> 14][address & 0x3FFF] = value;
    }

    inline void loadRamPage(uint8_t page, const uint8_t *data)
    {
        if (page < ram_pages && data != nullptr) {
            memcpy(ram_banks[page], data, page_size);
        }
    }

    // --- ROM Loading Helpers ---
    void loadRom(int bank_index, const uint8_t *rom_data, int rom_len)
    {
        if (bank_index < 0 || bank_index > 1) return;
        if (rom_len > 0x4000) rom_len = 0x4000;

        memcpy(ram_banks[bank_index], rom_data, rom_len);
        _print("mmu", "ROM Bank %d loaded: %d bytes", bank_index, rom_len);
    }

    // Backward compatibility helper if loading a single ROM file
    void loadRom(const uint8_t *rom_data, int rom_len)
    {
        loadRom(0, rom_data, rom_len);
    }

    // --- Direct Access Helpers ---
    inline uint16_t getPageSize()
    {
        // Returns the page size
        return page_size;
    }
    inline uint8_t *getScreenData()
    {
        // Returns pointer to whichever RAM bank is currently mapped as the screen (Bank 5 or 7)
        return read_pages[1];
    }

    inline uint8_t *getBankRam(int bank)
    {
        if (bank < 0 || bank > ram_pages) return nullptr;
        return ram_banks[bank];
    }

    inline uint8_t *getRamBankData(int bank)
    {
        if (bank < 0 || bank > ram_pages) return nullptr;
        return ram_banks[bank];
    }

    inline uint8_t *getFlatRam()
    {
        return ram_banks[1];
    }

    void dumpMemory() {
        uint16_t addr = 0;
        for (int i = 0; i < ram_pages; i++) {
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
/*
 * Original Author: Magnus Krook
 * Original Source: https://softspectrum48.weebly.com/
 * Original License: freeware
 *
 * Adapted by:      johndeckardsoftware
 * Adaptation Date: November 2026
 * Modifications:   Adapted for ESP32-S3-Touch-LCD-3.5_Rev2.0 board using ESP32/ESP-IDF platform.
 *
 */

 //#pragma once
#include <vector>
#include <cstdint>
#include "zx48k.h" // Includes ZXSpectrum, Z80, and Memory definitions

class TapeFlashLoader
{
public:
    // https://skoolkid.github.io/rom/asm/0556.html
    // https://retroisle.com/sinclair/zxspectrum/technical/firmware/tape_loading_routine.php
    static int FlashLoad(Z80 &z80, Memory &memory, TapeManager &tapeManager) {
        // the return value, representing the index of the tape_block which was flash loaded.
        uint16_t lastBlockIndex = -1;

        // if we encounter a block with no data to load (for example a text block), just skip ahead.
        if (tapeManager.GetNextBlock() != nullptr && tapeManager.GetNextBlock()->BlockContent.empty()) {
            printf("\n Skipped BlockType=%d\n", tapeManager.GetNextBlock()->BlockTypeNum);
            return lastBlockIndex;
        }

        // the ld bytes rom routine is intercepted at an early stage just before the edge detection is started.
        // check that the tape position is at the end of a block and that there is a following block to flash load.
        //if z80.pc == 0x056b and tapeManager.GetNextBlock() is not None and self.current_tape_position > tapeManager.GetNextBlock().start_position - 10:
        if (tapeManager.GetNextBlock() != nullptr && tapeManager.CurrentTapePosition > tapeManager.GetNextBlock()->StartPosition - 10) {
            // Get expected block type and load vs verify flag from af'
            uint8_t expectedBlockType = z80.reg.back.A;
            bool shouldLoad = z80.reg.back.F & 0x01; // LOAD rather than VERIFY
            //printf("\nexpectedBlockType=%d, shouldLoad=%d\n", expectedBlockType, shouldLoad);

            // Target address stored in IX
            uint16_t dataTargetParameter = z80.reg.IX;
            //printf("dataTargetParameter=%d\n", dataTargetParameter);

            // Block length stored in DE pair (`pair.D` and `pair.E`)
            uint16_t dataLengthParameter = (static_cast<uint16_t>(z80.reg.pair.D) << 8) | z80.reg.pair.E;
            //printf("dataLengthParameter=%d\n", dataLengthParameter);

            uint8_t actualBlockType = tapeManager.GetNextBlock()->BlockTypeNum;
            //printf("actualBlockType=%d\n", actualBlockType);

            const auto &blockContent = tapeManager.GetNextBlock()->BlockContent;
            size_t actualBlockLen = blockContent.size();
            //printf("actualBlockLen=%d\n", actualBlockLen);

            bool success = true;
            if (expectedBlockType != actualBlockType) {
                success = false;
            }
            else
            {
                if (shouldLoad)
                {
                    uint16_t offset = 0;
                    uint16_t loadedBytes = 0;
                    uint8_t checksum = actualBlockType;
                    uint8_t byte;
                    while (loadedBytes < dataLengthParameter)
                    {
                        if (offset >= actualBlockLen)
                        {
                            // have run out of bytes to load
                            success = false;
                            //printf("offset >= contentLength: success=%d, offset=%d, contentLength=%d\n", success, offset, actualBlockLen);
                            break;
                        }
                        byte = blockContent[offset];
                        offset += 1;
                        loadedBytes += 1;
                        memory.write(dataTargetParameter, byte);
                        dataTargetParameter++;
                        checksum ^= byte;
                    }

                    // if loading is going right, we should still have a checksum byte left to read
                    //printf("success=%d, offset=%d, contentLength=%d, dataTargetParameter=%d\n", success, offset, actualBlockLen, dataTargetParameter);
                    success &= (offset == actualBlockLen);
                    //printf("success (1)=%d\n", success);
                    if (success)
                    {
                        uint8_t expectedChecksum = tapeManager.GetNextBlock()->Checksum;
                        success = checksum == expectedChecksum;
                    }
                    else
                    {
                        // VERIFY. TODO: actually verify.
                        success = true;
                    }
                    //printf("success (2)=%d\n", success);
                }
                if (success)
                {
                    // set carry to indicate success
                    z80.reg.pair.F = z80.reg.pair.F | 0x01;
                    z80.reg.IX = dataTargetParameter;
                    z80.reg.pair.D = 0;
                    z80.reg.pair.E = 0;
                }
                else
                {
                    // reset carry to indicate failure
                    z80.reg.pair.F = z80.reg.pair.F | 0xfe;
                }
            }

            // keep track of the index of the last loaded tape block. this information
            // can be used to rewind the tape to the start position of the next block
            // after an auto pause.
            lastBlockIndex = tapeManager.GetNextBlock()->Index;
            tapeManager.GoToEndOfBlock(tapeManager.GetNextBlock()->Index);

            // Skip to end of LD BYTES ROM routine (RET)
            z80.reg.PC = 0x05E2;
        }

        return lastBlockIndex;
    }
};
#/*
 * Original Author: Magnus Krook
 * Original Source: https://softspectrum48.weebly.com/
 * Original License: freeware
 *
 * Adapted by:      johndeckardsoftware
 * Adaptation Date: November 2026
 * Modifications:   Adapted for ESP32-S3-Touch-LCD-3.5_Rev2.0 board using ESP32/ESP-IDF platform.
 *
 */

#ifndef TAPE_BLOCK_H
#define TAPE_BLOCK_H

#include <cstdint>
#include <string>
#include <vector>

class TapeBlock {
public:
    enum class BlockType {
        Program_header,
        Number_array_header,
        Character_array_header,
        Code_header,
        Data_block,
        Info,
        Unassigned
    };

    int Index = 0;
    BlockType Type = BlockType::Unassigned;
    std::string FileName;
    std::vector<uint8_t> BlockContent;
    bool IsHeaderless = false;
    int Checksum = 0;
    int BlockTypeNum = 0;
    int StartPosition = 0;
    int NumberOfPulses = 0;

    std::string GetBlockLength() const {
        return std::to_string(BlockContent.size());
    }
};

#endif // TAPE_BLOCK_H
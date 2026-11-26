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

#include <cstdio>
#include "tape.h"
#include "../common/helper.h"

static const char* TAG = "tape";

Tape::Tape(const std::vector<uint8_t>& fileContent)
    : _fileContent(fileContent), _fileContentIndex(0), _tapeListingIndex(0), _blockLength(0)
{
}

bool Tape::UpdateTapeListing(int blockLength, int fileContentIndex, int dataIndexAdjustment, int tapeListingIndex, int signalIndex)
{
    if (fileContentIndex + blockLength > static_cast<int>(_fileContent.size()))
    {
        FileError();
        return false;
    }

    int blockContentIndex = fileContentIndex + dataIndexAdjustment;

    int flagByte;
    if (blockContentIndex + 2 < static_cast<int>(_fileContent.size()))
    {
        flagByte = _fileContent[blockContentIndex + 2];
    }
    else
    {
        flagByte = 255;
    }

    if (flagByte == 0 && blockLength > 19)
    {
        flagByte = 0;
    }

    if (flagByte == 0 && blockLength == 19)
    {
        // Header block
        TapeBlock::BlockType dataBlockType;
        uint8_t typeByte = _fileContent[blockContentIndex + 3];
        switch (typeByte)
        {
            case 0:
                dataBlockType = TapeBlock::BlockType::Program_header;
                break;
            case 1:
                dataBlockType = TapeBlock::BlockType::Number_array_header;
                break;
            case 2:
                dataBlockType = TapeBlock::BlockType::Character_array_header;
                break;
            case 3:
                dataBlockType = TapeBlock::BlockType::Code_header;
                break;
            default:
                dataBlockType = TapeBlock::BlockType::Unassigned;
                break;
        }

        std::string fileName = "";
        for (int i = 0; i < 10; i++)
        {
            int asciiCode = _fileContent[blockContentIndex + 4 + i];
            fileName += static_cast<char>(asciiCode);
        }

        std::vector<uint8_t> blockContent(17);
        for (int i = 0; i < 17; i++)
        {
            blockContent[i] = _fileContent[blockContentIndex + 3 + i];
        }

        int checksum = _fileContent[blockContentIndex + 20];

        TapeBlock block;
        block.FileName = fileName;
        block.Type = dataBlockType;
        block.StartPosition = signalIndex;
        block.Index = tapeListingIndex;
        block.BlockContent = blockContent;
        block.IsHeaderless = false;
        block.Checksum = checksum;
        block.BlockTypeNum = 0;

        TapeListing.push_back(block);
    }
    else
    {
        // Data block
        int contentLength;
        int contentOffset;
        if (blockLength >= 2)
        {
            contentLength = blockLength - 2;
            contentOffset = 3;
        }
        else
        {
            contentLength = blockLength;
            contentOffset = 2;
        }

        std::vector<uint8_t> blockContent(contentLength);
        for (int i = 0; i < contentLength; i++)
        {
            blockContent[i] = _fileContent[blockContentIndex + contentOffset + i];
        }

        bool isHeaderless = false;
        if (tapeListingIndex > 0 && tapeListingIndex - 1 < static_cast<int>(TapeListing.size()))
        {
            if (TapeListing[tapeListingIndex - 1].Type == TapeBlock::BlockType::Data_block)
            {
                isHeaderless = true;
            }
        }

        int checksum = 0;
        if (blockContentIndex + 1 + blockLength < static_cast<int>(_fileContent.size()))
        {
            checksum = _fileContent[blockContentIndex + 1 + blockLength];
        }

        TapeBlock block;
        block.Type = TapeBlock::BlockType::Data_block;
        block.StartPosition = signalIndex;
        block.Index = _tapeListingIndex;
        block.BlockContent = blockContent;
        block.IsHeaderless = isHeaderless;
        block.Checksum = checksum;
        block.BlockTypeNum = flagByte;

        TapeListing.push_back(block);
    }
    return true;
}

void Tape::ApplyLeaderAndSyncPulses(uint32_t numberOfLeaderPulses, uint32_t leaderPulseLength, uint32_t firstSyncPulseLength, uint32_t secondSyncPulseLength)
{
    Signals.push_back(TapeSignal{numberOfLeaderPulses, false, false, false, leaderPulseLength, false, true});
    Signals.push_back(TapeSignal{1, false, false, false, firstSyncPulseLength, false, false});
    Signals.push_back(TapeSignal{1, false, false, false, secondSyncPulseLength, false, false});
}

void Tape::ConvertBlockData(int blockLength, int fileContentIndex, uint32_t zeroBitPulseLength, uint32_t oneBitPulseLength, int lastBits)
{
    for (int i = 0; i < blockLength; i++)
    {
        uint8_t value = _fileContent[fileContentIndex + i];
        int usedBits = (i == blockLength - 1) ? lastBits : 8;

        for (int bitIndex = 0; bitIndex < usedBits; bitIndex++)
        {
            int bitvalue = (value & 128) / 128;
            if (bitvalue == 0)
            {
                Signals.push_back(TapeSignal{2, false, false, false, zeroBitPulseLength, false, false});
            }
            else
            {
                Signals.push_back(TapeSignal{2, false, false, false, oneBitPulseLength, false, false});
            }

            value <<= 1;
        }
    }
}

void Tape::AddPause(uint32_t pauseLength)
{
    if (pauseLength > 0)
    {
        Signals.push_back(TapeSignal{1, true, false, false, pauseLength * TStatesPerMilliSecond, false, false});
    }
}

uint8_t Tape::GetByte(int offset) const
{
    return _fileContent[_fileContentIndex + offset];
}

uint32_t Tape::GetWord(int offset) const
{
    return static_cast<uint32_t>(_fileContent[_fileContentIndex + offset] + 256 * _fileContent[_fileContentIndex + 1 + offset]);
}

int Tape::GetLongWord(int offset) const
{
    return _fileContent[_fileContentIndex + offset] + 256 * _fileContent[_fileContentIndex + 1 + offset] + 65536 * _fileContent[_fileContentIndex + 2 + offset];
}

int Tape::GetDoubleWord(int offset) const
{
    return _fileContent[_fileContentIndex + offset] + 256 * _fileContent[_fileContentIndex + 1 + offset] + 65536 * _fileContent[_fileContentIndex + 2 + offset] + 16777216 * _fileContent[_fileContentIndex + 3 + offset];
}

bool Tape::CheckForEOF(int value) const
{
    if (value > static_cast<int>(_fileContent.size()))
    {
        const_cast<Tape*>(this)->FileError();
        return true;
    }
    return false;
}

void Tape::FileError()
{
    _print(TAG, "File read error.");
}

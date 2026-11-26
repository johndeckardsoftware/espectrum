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

#include "tzx_tape.h"
#include <algorithm>

TZXTape::TZXTape(const std::vector<uint8_t>& fileContent)
    : Tape(fileContent)
{
    ProcessTape();
}

void TZXTape::ProcessTape() {
    if (_fileContent.empty())
        return;

    uint32_t pauseLength;
    uint32_t leaderPulses;
    uint32_t leaderPulseLength;
    uint32_t firstSyncPulseLength;
    uint32_t secondSyncPulseLength;
    uint32_t zeroBitPulseLength;
    uint32_t oneBitPulseLength;
    int lastBits;
    int loopTimes = 0;
    int loopStart = 0;
    int loopEnd;
    int tZXblockType;

    // Skip the first TZX header ("ZXTape!\x1a")
    _fileContentIndex = 10;

    // Lambda helper mirroring the local C# function for text blocks
    auto updateTapeListingFromTZXTextItem = [this]() {
        std::string blockName = "";
        int textLength = _fileContent[_fileContentIndex];
        for (int i = 0; i < textLength; i++) {
            int asciiCode = _fileContent[_fileContentIndex + 1 + i];
            blockName += static_cast<char>(asciiCode);
        }

        TapeBlock block;
        block.FileName = blockName;
        block.StartPosition = static_cast<int>(Signals.size());
        block.Index = _tapeListingIndex;
        TapeListing.push_back(block);
    };

    do {
        if (_fileContentIndex >= static_cast<int>(_fileContent.size()))
            break;

        // Identify the current block type.
        tZXblockType = GetByte(0);
        _fileContentIndex++;

        // Keep track of where in the file listing we are.
        int currentFileListingIndex = _tapeListingIndex;

        // Handle the current block
        switch (tZXblockType) {
            // Standard speed data block
            case 0x10: {
                pauseLength = GetWord(0x00);
                _blockLength = static_cast<int>(GetWord(0x02));

                if (_blockLength == 19) {
                    UpdateTapeListing(_blockLength, _fileContentIndex, DataIndexAdjustment, _tapeListingIndex, static_cast<int>(Signals.size()));
                    _tapeListingIndex += 1;
                    ApplyLeaderAndSyncPulses(HeaderLeaderPulses, LeaderPulseLength, FirstSyncPulseLength, SecondSyncPulseLength);
                } else {
                    UpdateTapeListing(_blockLength, _fileContentIndex, DataIndexAdjustment, _tapeListingIndex, static_cast<int>(Signals.size()));
                    _tapeListingIndex += 1;
                    ApplyLeaderAndSyncPulses(DataLeaderPulses, LeaderPulseLength, FirstSyncPulseLength, SecondSyncPulseLength);
                }

                _fileContentIndex += 4;
                ConvertBlockData(_blockLength, _fileContentIndex, ZeroBitPulseLength, OneBitPulseLength, 8);
                AddPause(pauseLength);
                _fileContentIndex += _blockLength;
                break;
            }

            // Turbo speed data block.
            case 0x11: {
                leaderPulseLength = GetWord(0x00);
                firstSyncPulseLength = GetWord(0x02);
                secondSyncPulseLength = GetWord(0x04);
                zeroBitPulseLength = GetWord(0x06);
                oneBitPulseLength = GetWord(0x08);
                leaderPulses = GetWord(0x0A);
                lastBits = GetByte(0x0C);
                pauseLength = GetWord(0x0D);
                _blockLength = GetLongWord(0x0F);

                UpdateTapeListing(_blockLength, _fileContentIndex, 16, _tapeListingIndex, static_cast<int>(Signals.size()));
                _tapeListingIndex += 1;

                ApplyLeaderAndSyncPulses(leaderPulses, leaderPulseLength, firstSyncPulseLength, secondSyncPulseLength);
                _fileContentIndex += 18;

                ConvertBlockData(_blockLength, _fileContentIndex, zeroBitPulseLength, oneBitPulseLength, lastBits);
                AddPause(pauseLength);
                _fileContentIndex += _blockLength;
                break;
            }

            // Pure tone.
            case 0x12: {
                Signals.push_back(TapeSignal{GetWord(0x02), false, false, false, GetWord(0x00), false, false});
                _fileContentIndex += 4;
                break;
            }

            // Pulse sequence.
            case 0x13: {
                int numberOfPulses = GetByte(0x00);
                for (int pulse = 0; pulse < numberOfPulses; pulse++) {
                    Signals.push_back(TapeSignal{1, false, false, false, GetWord(0x01), false, false});
                    _fileContentIndex += 2;
                }
                _fileContentIndex += 1;
                break;
            }

            // Pure data block.
            case 0x14: {
                zeroBitPulseLength = GetWord(0x00);
                oneBitPulseLength = GetWord(0x02);
                lastBits = GetByte(0x04);
                pauseLength = GetWord(0x05);
                _blockLength = static_cast<int>(GetLongWord(0x07));

                _fileContentIndex += 10;
                ConvertBlockData(_blockLength, _fileContentIndex, zeroBitPulseLength, oneBitPulseLength, lastBits);
                AddPause(pauseLength);
                _fileContentIndex += _blockLength;
                break;
            }

            // Stop the tape or pause.
            case 0x20: {
                pauseLength = GetWord(0x00);
                if (pauseLength > 0) {
                    Signals.push_back(TapeSignal{1, false, false, false, static_cast<uint32_t>(TStatesPerMilliSecond), false, false});
                    Signals.push_back(TapeSignal{1, true, false, false, pauseLength * TStatesPerMilliSecond - TStatesPerMilliSecond, false, false});
                } else {
                    Signals.push_back(TapeSignal{0, false, false, false, 0, true, false});
                }
                _fileContentIndex += 2;
                break;
            }

            // Group or meta block.
            case 0x21: {
                updateTapeListingFromTZXTextItem();
                _tapeListingIndex += 1;
                _fileContentIndex = _fileContentIndex + GetByte(0x00) + 1;
                break;
            }

            // End of group or meta block
            case 0x22:
                break;

            // Loop.
            case 0x24: {
                loopTimes = static_cast<int>(GetWord(0x00));
                loopStart = static_cast<int>(Signals.size());
                _fileContentIndex += 2;
                break;
            }

            // Loop end.
            case 0x25: {
                loopEnd = static_cast<int>(Signals.size()) - 1;
                if (loopEnd >= loopStart) {
                    auto rangeStart = Signals.begin() + loopStart;
                    auto rangeEnd = Signals.begin() + loopEnd + 1;
                    std::vector<TapeSignal> loopBlock(rangeStart, rangeEnd);

                    for (int loopRepeat = 0; loopRepeat < loopTimes - 1; loopRepeat++) {
                        Signals.insert(Signals.end(), loopBlock.begin(), loopBlock.end());
                    }
                }
                break;
            }

            // Text description.
            case 0x30: {
                if (_tapeListingIndex > 0) {
                    updateTapeListingFromTZXTextItem();
                    _tapeListingIndex += 1;
                }
                _fileContentIndex = _fileContentIndex + GetByte(0x00) + 1;
                break;
            }

            // Archive info.
            case 0x32:
                _fileContentIndex = _fileContentIndex + static_cast<int>(GetWord(0x00)) + 2;
                break;

            // Hardware info.
            case 0x33:
                _fileContentIndex = _fileContentIndex + GetByte(0x00) * 3 + 1;
                break;

            // Custom info block.
            case 0x35: {
                int length = GetLongWord(0x10);
                _fileContentIndex = _fileContentIndex + 16 + length + 4;
                break;
            }

            // Skip block.
            case 0x5A:
                _fileContentIndex += 9;
                break;

            default:
                // Unknown block types: skip safely or break if out of sync
                break;
        }

        // Store the number of pulses generated in the block data
        if (currentFileListingIndex != _tapeListingIndex && !TapeListing.empty()) {
            TapeListing[_tapeListingIndex - 1].NumberOfPulses = static_cast<int>(Signals.size()) - TapeListing[_tapeListingIndex - 1].StartPosition;
        }
    }
    while (_fileContentIndex < static_cast<int>(_fileContent.size()));

    // Add an edge to finish the last pulse.
    //Signals.push_back(TapeSignal{1, false, false, false, 500000, false, false});
}

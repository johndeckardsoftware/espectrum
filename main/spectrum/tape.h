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

#ifndef TAPE_H
#define TAPE_H

#include <cstdint>
#include <vector>
#include <string>
#include "tape_block.h"
#include "tape_signal.h"

class Tape {
protected:
    static constexpr int DataIndexAdjustment = 0;
    static constexpr int HeaderLeaderPulses = 8063;
    static constexpr int DataLeaderPulses = 3223;
    static constexpr int LeaderPulseLength = 2168;
    static constexpr int FirstSyncPulseLength = 667;
    static constexpr int SecondSyncPulseLength = 735;
    static constexpr int ZeroBitPulseLength = 855;
    static constexpr int OneBitPulseLength = 1710;
    static constexpr int TStatesPerMilliSecond = 3500;

    std::vector<uint8_t> _fileContent;
    int _fileContentIndex = 0;
    int _tapeListingIndex = 0;
    int _blockLength = 0;

    virtual void ProcessTape() = 0;

    bool UpdateTapeListing(int blockLength, int fileContentIndex, int dataIndexAdjustment, int tapeListingIndex, int signalIndex);
    void ApplyLeaderAndSyncPulses(uint32_t numberOfLeaderPulses, uint32_t leaderPulseLength, uint32_t firstSyncPulseLength, uint32_t secondSyncPulseLength);
    void ConvertBlockData(int blockLength, int fileContentIndex, uint32_t zeroBitPulseLength, uint32_t oneBitPulseLength, int lastBits);
    void AddPause(uint32_t pauseLength);

    uint8_t GetByte(int offset) const;
    uint32_t GetWord(int offset) const;
    int GetLongWord(int offset) const;
    int GetDoubleWord(int offset) const;
    bool CheckForEOF(int value) const;
    void FileError();

public:
    std::vector<TapeSignal> Signals;
    std::vector<TapeBlock> TapeListing;

    explicit Tape(const std::vector<uint8_t>& fileContent);
    virtual ~Tape() = default;
};

#endif // TAPE_H
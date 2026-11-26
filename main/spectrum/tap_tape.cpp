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

#include "tap_tape.h"

TAPTape::TAPTape(const std::vector<uint8_t>& fileContent)
    : Tape(fileContent)
{
    ProcessTape();
}

void TAPTape::ProcessTape() {
    if (_fileContent.empty()) {
        return;
    }

    while (_fileContentIndex + 1 < static_cast<int>(_fileContent.size())) {
        _blockLength = static_cast<uint16_t>(_fileContent[_fileContentIndex]) |
                       (static_cast<uint16_t>(_fileContent[_fileContentIndex + 1]) << 8);

        UpdateTapeListing(_blockLength, static_cast<int>(_fileContentIndex), 0, _tapeListingIndex, static_cast<int>(Signals.size()));
        _tapeListingIndex += 1;

        // Check if this is a header block (19 bytes)
        if (_blockLength == 19) {
            ApplyLeaderAndSyncPulses(HeaderLeaderPulses, LeaderPulseLength, FirstSyncPulseLength, SecondSyncPulseLength);
        } else {
            ApplyLeaderAndSyncPulses(DataLeaderPulses, LeaderPulseLength, FirstSyncPulseLength, SecondSyncPulseLength);
        }

        _fileContentIndex += 2;

        // Guard against truncated files
        if (_fileContentIndex + _blockLength > static_cast<int>(_fileContent.size())) {
            break;
        }

        // Process the block data
        ConvertBlockData(_blockLength, _fileContentIndex, ZeroBitPulseLength, OneBitPulseLength, 8);

        _fileContentIndex += _blockLength;

        // Add a 2s pause after the block (except the last block)
        if (_fileContentIndex <= static_cast<int>(_fileContent.size())) {
            AddPause(1000);
            AddPause(1000);
        }

        // Store the number of pulses generated in the block data
        TapeListing[_tapeListingIndex - 1].NumberOfPulses =
            static_cast<int>(Signals.size()) - TapeListing[_tapeListingIndex - 1].StartPosition;
    }
}
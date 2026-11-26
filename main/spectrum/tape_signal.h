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

#ifndef TAPE_SIGNAL_H
#define TAPE_SIGNAL_H

#include <cstdint>

struct TapeSignal {
    uint32_t NumberOfPulses = 0;
    bool Pause = false;
    bool ForceHigh = false;
    bool ForceLow = false;
    uint32_t PulseLength = 0;
    bool StopTape = false;
    bool IsLeader = false;
};

#endif // TAPE_SIGNAL_H
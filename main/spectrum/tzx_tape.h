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

#ifndef TZX_TAPE_H
#define TZX_TAPE_H

#include "tape.h"

class TZXTape : public Tape {
protected:
    static constexpr int DataIndexAdjustment = 2;
    void ProcessTape() override;

public:
    explicit TZXTape(const std::vector<uint8_t>& fileContent);
};

#endif // TZX_TAPE_H
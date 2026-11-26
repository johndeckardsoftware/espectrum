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

 #ifndef TAP_TAPE_H
#define TAP_TAPE_H

#include "tape.h"

class TAPTape : public Tape {
protected:
    void ProcessTape() override;

public:
    explicit TAPTape(const std::vector<uint8_t>& fileContent);
};

#endif // TAP_TAPE_H
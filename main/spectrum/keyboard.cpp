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

#include "keyboard.h"
#include <stdio.h>

// Initialize the static lookup table mapping SpecKeys enum to row index and bitmask
const int Keyboard::key2specy[2][41] = {
    {0, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,
     2, 2, 2, 2, 2, 5, 5, 5, 5, 5,
     1, 1, 1, 1, 1, 6, 6, 6, 6, 6,
     0, 0, 0, 0, 0, 7, 7, 7, 7, 7},
    {0, 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
     0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
     0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
     0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE}};

Keyboard::Keyboard()
    : readCounter(0),
      pressHoldDuration(16),        // Default window: 16 port reads minimum
      releaseHoldDuration(16),      // Default window: 16 port reads minimum
      holdReadsCounter(0)
{
    reset();
}

void Keyboard::reset()
{
    for (int i = 0; i < 8; i++)
    {
        speckey[i] = 0xFF;
    }

    readCounter = 0;

    // Clear standard event queue
    std::queue<KeyEvent> emptyQueue;
    std::swap(eventQueue, emptyQueue);
}

void Keyboard::setDelays(uint32_t pressReads, uint32_t releaseReads)
{
    pressHoldDuration = pressReads;
    releaseHoldDuration = releaseReads;
}

void Keyboard::updateKey(SpecKeys key, uint8_t state)
{
    eventQueue.push({key, state});
    //printf("updateKey(SpecKeys %d, uint8_t %d): tick:%d, size:%d\n", key, state, 0, eventQueue.size());
}

void Keyboard::applyKeyEvent(SpecKeys key, uint8_t state)
{
    if (key < SPECKEY_MAX_NORMAL)
    {
        int row = key2specy[0][key];
        uint8_t mask = key2specy[1][key];

        if (state == 1)
        {
            speckey[row] &= mask; // Press: clear bit
        }
        else
        {
            speckey[row] |= (mask ^ 0xFF); // Release: set bit
        }
    }
}

uint8_t Keyboard::flushAndReadMatrix(uint8_t portHigh)
{
    // Increment the internal clock for I/O requests
    readCounter++;

    // Process all primary application inputs from the main queue
    if (holdReadsCounter == 0) {
        if (!eventQueue.empty())
        {
            KeyEvent ev = eventQueue.front();
            eventQueue.pop();

            uint16_t delay = (ev.state == 1) ? pressHoldDuration : releaseHoldDuration;
            applyKeyEvent(ev.key, ev.state);
            holdReadsCounter = delay;
        }
    }
    else {
        holdReadsCounter--;
    }

    // Decode keyboard matrix rows based on active-low address lines (A8-A15)
    uint8_t data = 0xBF;
    if (!(portHigh & 0x01)) data &= speckey[0];
    if (!(portHigh & 0x02)) data &= speckey[1];
    if (!(portHigh & 0x04)) data &= speckey[2];
    if (!(portHigh & 0x08)) data &= speckey[3];
    if (!(portHigh & 0x10)) data &= speckey[4];
    if (!(portHigh & 0x20)) data &= speckey[5];
    if (!(portHigh & 0x40)) data &= speckey[6];
    if (!(portHigh & 0x80)) data &= speckey[7];

    return data;
}

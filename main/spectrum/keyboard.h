#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <queue>
#include "keyboard_defs.h"

/**
 * @brief Manages the ZX Spectrum 8x5 keyboard matrix emulation with a delayed event queue
 *        to handle rapid virtual/touchscreen input timing.
 */
class Keyboard
{
private:
    struct KeyEvent {
        SpecKeys key;   ///< The specific Spectrum key.
        uint8_t state;  ///< Input state (1 = Pressed, 0 = Released).
    };

    struct DelayedRelease {
        SpecKeys key;          ///< The key to be released.
        uint32_t releaseTick;  ///< The readCounter value at which this release becomes valid.
    };

    // Keyboard matrix state (8 rows, 5 bits each). 0xFF means unpressed.
    // row0: SHIFT, Z, X, C, V  | row1: A, S, D, F, G
    // row2: Q, W, E, R, T      | row3: 1, 2, 3, 4, 5
    // row4: 0, 9, 8, 7, 6      | row5: P, O, I, U, Y
    // row6: ENTER, L, K, J, H  | row7: SPACE, SYMB, M, N, B
    uint8_t speckey[8];

    // Hardware matrix lookup table mapping SpecKeys to rows and bitmasks
    static const int key2specy[2][41];

    // Main queue for incoming real-time application inputs
    std::queue<KeyEvent> eventQueue;

    // Global counter incremented on every Port 0xFE read request
    uint32_t readCounter;

    // Configurable duration (in number of port reads) a key must remain pressed/released
    uint32_t pressHoldDuration;
    uint32_t releaseHoldDuration;

    // Key hold counter decremented on every Port 0xFE read request
    uint16_t holdReadsCounter;

    /**
     * @brief Internal method to apply a single event directly to the hardware matrix.
     */
    void applyKeyEvent(SpecKeys key, uint8_t state);

public:
    /**
     * @brief Construct a new Keyboard object with a default hold duration.
     */
    Keyboard();

    /**
     * @brief Reset the keyboard matrix state and clear all queues.
     */
    void reset();

    /**
     * @brief Sets the minimum duration a keypress must persist in terms of I/O reads.
     * @param pressReads Number of port reads required to hold press.
     * @param releaseReads Number of port reads required to hold release.
     */
    void setDelays(uint32_t pressReads, uint32_t releaseReads);

    /**
     * @brief Pushes a key event into the processing queue.
     * @param key The specific spectrum key enum to update.
     * @param state Input state (1 = Pressed, 0 = Released).
     */
    void updateKey(SpecKeys key, uint8_t state);

    /**
     * @brief Flushes qualified events, increments read counters, and reads the matrix.
     * @param portHigh The high byte of the 16-bit I/O port address (A8-A15).
     * @return uint8_t The combined 5-bit matrix data (active low).
     */
    uint8_t flushAndReadMatrix(uint8_t portHigh);
};

#endif // KEYBOARD_H

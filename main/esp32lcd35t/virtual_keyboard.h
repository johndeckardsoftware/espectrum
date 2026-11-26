/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <vector>
#include <functional>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"
#include "spectrum/keyboard_defs.h"
#include "spectrum/keyboard.h"

#define KEY_PRESSED     1
#define KEY_RELEASED    0

// Structural Custom Application Key Event Codes
#define KEY_MENU 28
#define KEY_SOUND 29
#define KEY_DEBUG 30
#define KEY_RUN 31
#define KEY_LOAD 27
#define KEY_LAYOUT_SWAP 26  // Custom code to cycle layouts
#define KEY_TEST 25         // Hook for test code
#define KEY_USB_MSC 24      // Toggle USB Mass Storage
#define KEY_SCREENSHOT 23   // Screenshot
#define KEY_RESET 22        // Reset
#define KEY_PAD_UP 21
#define KEY_PAD_DOWN 20
#define KEY_PAD_LEFT 19
#define KEY_PAD_RIGHT 18
#define KEY_PAD_FIRE 17

/**
 * @brief Available virtual keyboard layout configurations.
 */
enum KeyboardLayoutType
{
    LAYOUT_FULL_QWERTY = 0, // Default full keyboard layout
    LAYOUT_GAMEPAD_QAOP,    // D-Pad (Up, Down, Left, Right) + Fire + Quick Utility keys
    LAYOUT_GAMEPAD_5678,    // D-Pad (Up, Down, Left, Right) + Fire + Quick Utility keys
    LAYOUT_GAMEPAD_USER,    // D-Pad (Up, Down, Left, Right) + Fire + Quick Utility keys
    LAYOUT_CONFIG,          // Configuration (USB Mass Storage, Virtual game pad)
    LAYOUT_MAX_TYPES        // Sentinel for cycling
};

/**
 * @brief Bitmask enumerations representing persistent touchscreen modifier states.
 */
enum ModifierState
{
    MOD_NONE         = 0,
    MOD_CAPS_SHIFT   = 1 << 0,
    MOD_CAPS_LOCK    = 1 << 1,
    MOD_SYMBOL_SHIFT = 1 << 2
};

/**
 * @brief Bounding box properties defining a single rendered virtual touch key.
 */
struct KeyDef
{
    int16_t x, y, w, h;     ///< Position coordinates and structural sizes.
    const char *label;      ///< Text string rendered inside the button box.
    char base_val;          ///< Character code emitted under normal phase conditions.
    char shift_val;         ///< Character code emitted when shift structures are active.
    bool is_sticky;         ///< Flag forcing persistent toggle actions instead of instantaneous push.
};

/**
 * @brief Subsystem class managing on-screen visual keyboards layouts and raw touch routing.
 */
class VirtualKeyboard
{
private:
    lgfx::LGFX_Device &lcd;   ///< Shared hardware target display abstraction layout link.
    LGFX_Sprite sprLeft;     ///< Sprite canvas dedicated to rendering the left control sidebar.
    LGFX_Sprite sprRight;    ///< Sprite canvas dedicated to rendering the right system utilities sidebar.
    LGFX_Sprite sprBottom;   ///< Sprite canvas managing the main keyboard grid area.

    uint8_t modifiers;       ///< Track bitfields matching active status of Shift, Lock, Symb flags.
    KeyboardLayoutType currentLayout; ///< Active layout profile identifier.

    std::vector<KeyDef> leftKeys;
    std::vector<KeyDef> rightKeys;
    std::vector<KeyDef> bottomKeys;

    const char * nvsPadUp = "key_pad_up";
    int8_t keyPadUp = 'q';
    const char * nvsPadDown = "key_pad_down";
    int8_t keyPadDown = 'a';
    const char * nvsPadLeft = "key_pad_left";
    int8_t keyPadLeft = 'o';
    const char * nvsPadRight = "key_pad_right";
    int8_t keyPadRight = 'p';
    const char * nvsPadFire = "key_pad_fire";
    int8_t keyPadFire = ' ';
    int8_t * keyPadToBeAssigned = nullptr;
    const char * nvsPadToBeAssigned = nullptr;

    Keyboard* emuKeyboard;
    const char * keyPHD = "keyPHD";
    int8_t pressHoldDuration;      // Default window: 16 port reads minimum for standard keyboard, 2 for gamepad layout
    const char * keyRHD = "keyRHD";
    int8_t releaseHoldDuration;    // Default window: 16 port reads minimum  for standard keyboard, 2 for gamepad layout

    /// Registered client callback executed whenever a functional non-sticky text action resolves.
    std::function<void(char key, uint8_t modifiers, bool pressed)> onKeyEvent;

    void drawPanel(LGFX_Sprite &spr, const std::vector<KeyDef> &keys);
    void checkKeyHit(int16_t relX, int16_t relY, const std::vector<KeyDef> &keys, bool isPressed);

    // Layout Generators
    void setupQwerty();
    void buildQwertyLayout();
    void buildGamepadQAOP();
    void buildGamepad5678();
    void buildGamepadUser();
    void buildConfigLayout();

public:
    VirtualKeyboard(lgfx::LGFX_Device &display, std::function<void(char, uint8_t, bool)> callback);

    void init();
    void redraw();
    void handleTouch(int16_t x, int16_t y, bool isPressed);
    inline const uint16_t* getLeftBuffer() const   { return (const uint16_t*)sprLeft.getBuffer(); }
    inline const uint16_t* getRightBuffer() const  { return (const uint16_t*)sprRight.getBuffer(); }
    inline const uint16_t* getBottomBuffer() const { return (const uint16_t*)sprBottom.getBuffer(); }

    /**
     * @brief Set the emulator keyboard driver.
     */
    void setEmuKeyboard(Keyboard* kb);

    /**
    * @brief Cycles to the next available keyboard layout (QWERTY -> Gamepad -> Blank -> QWERTY).
     */
    void cycleLayout();

    /**
     * @brief Directly sets a specific layout profile.
     */
    void setLayout(KeyboardLayoutType layout);

    inline uint8_t getModifiers() const { return modifiers; }
    inline KeyboardLayoutType getCurrentLayout() const { return currentLayout; }
};

#endif // VIRTUAL_KEYBOARD_H
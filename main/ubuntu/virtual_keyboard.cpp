/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

 #include "virtual_keyboard.h"
#include "audio.h"
#include "esp32lcd35t/platform_log.h"
#include "esp_3inch5_lcd_port.h"

static const char *TAG = "vkb";

extern int16_t debug_level;
extern AudioSystem audioSystem;

VirtualKeyboard::VirtualKeyboard(lgfx::LGFX_Device &display, std::function<void(char, uint8_t, bool)> callback)
    : lcd(display),
      sprLeft(&lcd),
      sprRight(&lcd),
      sprBottom(&lcd),
      modifiers(MOD_NONE),
      currentLayout(LAYOUT_FULL_QWERTY),
      onKeyEvent(callback)
{
}

void VirtualKeyboard::init()
{
    sprLeft.setColorDepth(16);
    sprRight.setColorDepth(16);
    sprBottom.setColorDepth(16);

    sprLeft.setPsram(true);
    sprRight.setPsram(true);
    sprBottom.setPsram(true);

    sprLeft.createSprite(96, 224);
    sprRight.createSprite(96, 224);
    sprBottom.createSprite(480, 96);

    // Load default layout
    setLayout(LAYOUT_FULL_QWERTY);
}

void VirtualKeyboard::setLayout(KeyboardLayoutType layout)
{
    currentLayout = layout;
    leftKeys.clear();
    rightKeys.clear();
    bottomKeys.clear();

    switch (currentLayout) {
        case LAYOUT_FULL_QWERTY:
            buildQwertyLayout();
            break;
        case LAYOUT_GAMEPAD_QAOP:
            buildGamepadQAOP();
            break;
        case LAYOUT_GAMEPAD_5678:
            buildGamepad5678();
            break;
        case LAYOUT_CONFIG:
            buildConfigLayout();
            break;
        default:
            buildQwertyLayout();
            break;
    }
    redraw();
}

void VirtualKeyboard::cycleLayout()
{
    int next = (int)currentLayout + 1;
    if (next >= LAYOUT_MAX_TYPES) {
        next = 0;
    }
    setLayout((KeyboardLayoutType)next);
    ESP_LOGI(TAG, "Switched to layout mode: %d", next);
}

void VirtualKeyboard::buildQwertyLayout()
{
    // Left Sidebar: System modifiers
    leftKeys = {
        {0, 0,   96, 32, "CAPS",   'L', 'L', true},
        {0, 32,  96, 32, "SHIFT",  'C', 'C', true},
        {0, 64,  96, 32, "SYMBOL", 'S', 'S', true},
        {0, 96,  96, 32, "DELETE", 8,   8,   false},
        {0, 128, 96, 32, "SPACE",  32,  32,  false},
        {0, 160, 96, 32, "ENTER",  10,  10,  false},
    };

    // Right Sidebar: Utilities + Layout Switcher
    rightKeys = {
        {0, 0,  96, 32, "LAYOUT", KEY_LAYOUT_SWAP, KEY_LAYOUT_SWAP, false}, // Tap to swap layouts!
        {0, 32, 96, 32, "MENU", KEY_MENU, KEY_MENU, false},
        {0, 64, 96, 32, "SOUND", KEY_SOUND, KEY_SOUND, true},
        {0, 96, 96, 32, "LOAD",  KEY_LOAD, KEY_LOAD, false},
        {0, 128,96, 32, "RUN",  KEY_RUN, KEY_RUN, false},
        {0, 160,96, 32, "DEBUG", KEY_DEBUG, KEY_DEBUG, true},
        {0, 192,96, 32, "test", KEY_TEST, KEY_TEST, false},
    };

    // Bottom Grid: Standard QWERTY rows
    const char *r0_lbl[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    char r0_base[]       = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
    char r0_shift[]      = {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};
    for (int i = 0; i < 10; ++i) bottomKeys.push_back({(int16_t)(i * 48), 0, 48, 24, r0_lbl[i], r0_base[i], r0_shift[i], false});

    const char *r1_lbl[] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"};
    char r1_base[]       = {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'};
    char r1_shift[]      = {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'};
    for (int i = 0; i < 10; ++i) bottomKeys.push_back({(int16_t)(i * 48), 24, 48, 24, r1_lbl[i], r1_base[i], r1_shift[i], false});

    const char *r2_lbl[] = {"A", "S", "D", "F", "G", "H", "J", "K", "L", "\n"};
    char r2_base[]       = {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '\n'};
    char r2_shift[]      = {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '\n'};
    for (int i = 0; i < 10; ++i) bottomKeys.push_back({(int16_t)(i * 48), 48, 48, 24, r2_lbl[i], r2_base[i], r2_shift[i], false});

    const char *r3_lbl[] = {"Z", "X", "C", "V", "B", "N", "M", ",", ".", "\b"};
    char r3_base[]       = {'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '\b'};
    char r3_shift[]      = {'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '\b'};
    for (int i = 0; i < 10; ++i) bottomKeys.push_back({(int16_t)(i * 48), 72, 48, 24, r3_lbl[i], r3_base[i], r3_shift[i], false});
}

void VirtualKeyboard::buildGamepadQAOP()
{
    /// Left Side: D-Pad Controls (Up, Down, Left, Right configuration mapped to keys or standard directions)
    /// Note: You can customize base_val to match specific game control schemes (e.g., Q/A/O/P for Sinclair, or custom keys)
    leftKeys = {
        {0, 0,  96, 32, "QUIT", 'x', 'x', false},
    ///    {0, 32,  96, 32, "", '.', '.', false},
    ///    {0, 64,  96, 32, "", '.', '.', false},
        {0, 96,  96, 128, "UP q", 'q', 'q', false}, // Example mapping: W/Q or custom
    };

    // Right Side: Fire button + Quick utility & Layout Swapper
    rightKeys = {
        {0, 0,  96, 32, "LAYOUT", KEY_LAYOUT_SWAP, KEY_LAYOUT_SWAP, false},
        {0, 32,   96, 32, "MENU",   KEY_MENU, KEY_MENU, false},
    ///    {0, 96,  96, 32, "SOUND",  KEY_SOUND, KEY_SOUND, true},
        {0, 96, 96, 128, "RIGHT p",  'P', 'P', false},
    };

    /// Bottom Grid: Actually three big button for left, fire, right or can be redefined
    /// for extra game action buttons (e.g., Bomb, Jump, Extra Fire)
    bottomKeys = {
        {0, 0, 160,  96, "LEFT o", 'o', 'o', false},
        {160, 0, 160, 96, "FIRE ( )", ' ', ' ', false},
        {320, 0, 160,  96, "DOWN a", 'a', 'a', false},
    };
}

void VirtualKeyboard::buildGamepad5678()
{
    /// Left Side: D-Pad Controls (Up, Down, Left, Right configuration mapped to keys or standard directions)
    /// Note: You can customize base_val to match specific game control schemes (e.g., Q/A/O/P for Sinclair, or custom keys)
    leftKeys = {
        {0, 0,  96, 32, "QUIT", 'q', 'q', false},
    ///    {0, 32,  96, 32, "", '.', '.', false},
    ///    {0, 64,  96, 32, "", '.', '.', false},
        {0, 96,  96, 128, "UP 7", '7', '7', false}, // Example mapping: W/Q or custom
    };

    // Right Side: Fire button + Quick utility & Layout Swapper
    rightKeys = {
        {0, 0,  96, 32, "LAYOUT", KEY_LAYOUT_SWAP, KEY_LAYOUT_SWAP, false},
        {0, 32, 96, 32, "MENU",   KEY_MENU, KEY_MENU, false},
    ///    {0, 96,  96, 32, "SOUND",  KEY_SOUND, KEY_SOUND, true},
        {0, 96, 96, 128, "RIGHT 8",  '8', '8', false},
    };

    /// Bottom Grid: Actually three big button for left, fire, right or can be redefined
    /// for extra game action buttons (e.g., Bomb, Jump, Extra Fire)
    bottomKeys = {
        {0,   0, 160, 96, "LEFT 5", '5', '5', false},
        {160, 0, 160, 96, "FIRE ( )", ' ', ' ', false},
        {320, 0, 160, 96, "DOWN 6", '6', '6', false},
    };
}

void VirtualKeyboard::buildConfigLayout()
{
    // Minimal / Blank layout: Removes clutter from sidebars and bottom, maximizing screen visibility.
    // Only a small "LAYOUT" indicator remains on the right edge so you can tap to bring controls back.
    rightKeys = {
        {0, 0, 96, 32, "LAYOUT", KEY_LAYOUT_SWAP, KEY_LAYOUT_SWAP, false},
    };
    leftKeys.clear();
    bottomKeys.clear();
}

void VirtualKeyboard::drawPanel(LGFX_Sprite &spr, const std::vector<KeyDef> &keys)
{
    spr.fillSprite(TFT_DARKGREY);
    spr.setTextSize(2);

    for (const auto &k : keys) {
        bool highlighted = false;

        if (k.is_sticky) {
            if (k.base_val == 'L' && (modifiers & MOD_CAPS_LOCK))     highlighted = true;
            if (k.base_val == 'C' && (modifiers & MOD_CAPS_SHIFT))    highlighted = true;
            if (k.base_val == 'S' && (modifiers & MOD_SYMBOL_SHIFT))  highlighted = true;
            if (k.base_val == KEY_DEBUG && debug_level > 0)           highlighted = true;
            if (k.base_val == KEY_SOUND && audioSystem.isSpeakerPowered())  highlighted = true;
        }

        if (k.base_val == KEY_MENU) highlighted = true;
        if (k.base_val == KEY_LOAD) highlighted = true;

        if (k.base_val == KEY_LAYOUT_SWAP) {
            spr.fillRect(k.x, k.y, k.w, k.h, TFT_RED);
        } else if (k.base_val == KEY_MENU) {
            spr.fillRect(k.x, k.y, k.w, k.h, TFT_YELLOW);
        } else if (k.base_val == KEY_SOUND) {
            spr.fillRect(k.x, k.y, k.w, k.h, TFT_GREEN);
        } else if (k.base_val == KEY_LOAD) {
            spr.fillRect(k.x, k.y, k.w, k.h, TFT_CYAN);
        } else {
            spr.fillRect(k.x, k.y, k.w, k.h, highlighted ? TFT_BLUE : TFT_LIGHTGREY);
        }

        spr.drawRect(k.x, k.y, k.w, k.h, TFT_BLACK);
        spr.setTextColor(k.base_val == KEY_LAYOUT_SWAP ? TFT_BLACK : (highlighted ? TFT_BLACK : TFT_WHITE));

        int16_t textW = spr.textWidth(k.label);
        int16_t textH = spr.fontHeight();
        int16_t cursorX = k.x + (k.w - textW) / 2;
        int16_t cursorY = k.y + (k.h - textH) / 2;

        spr.setCursor(cursorX, cursorY);
        spr.print(k.label);
    }
}

void VirtualKeyboard::redraw()
{
    sprLeft.fillSprite(TFT_DARKGREY);
    drawPanel(sprLeft, leftKeys);
    sprLeft.pushSprite(0, 0);

    sprRight.fillSprite(TFT_DARKGREY);
    drawPanel(sprRight, rightKeys);
    sprRight.pushSprite(384, 0);

    sprBottom.fillSprite(TFT_DARKGREY);
    drawPanel(sprBottom, bottomKeys);
    sprBottom.pushSprite(0, 224);
}

void VirtualKeyboard::handleTouch(int16_t x, int16_t y, bool isPressed)
{
    if (x >= 0 && x < 96 && y >= 0 && y < 224) {
        checkKeyHit(x, y, leftKeys, isPressed);
    }
    else if (x >= 384 && x < 480 && y >= 0 && y < 224) {
        checkKeyHit(x - 384, y, rightKeys, isPressed);
    }
    else if (x >= 0 && x < 480 && y >= 224 && y < 320) {
        checkKeyHit(x, y - 224, bottomKeys, isPressed);
    }
    else {
        // If clicking in the main game screen area while in blank/minimal mode,
        // you can toggle backlights or cycle layouts, or pass touch through.
        if (currentLayout == LAYOUT_CONFIG && isPressed) {
            cycleLayout(); // Tap screen to bring controls back if hidden
        } else if (currentLayout != LAYOUT_CONFIG) {
            uint8_t brightness = esp_3inch5_brightness_port_get();
            esp_3inch5_brightness_port_set(brightness > 0 ? 0 : 50);
        }
    }
}

void VirtualKeyboard::checkKeyHit(int16_t relX, int16_t relY, const std::vector<KeyDef> &keys, bool isPressed)
{
    for (const auto &k : keys) {
        if (relX >= k.x && relX < (k.x + k.w) && relY >= k.y && relY < (k.y + k.h)) {
            if (k.base_val == KEY_LAYOUT_SWAP && isPressed) {
                cycleLayout(); // Swap layout instantly when tapped
            }
            else if (k.is_sticky && isPressed) {
                switch (k.base_val)
                {
                case 'L': modifiers ^= MOD_CAPS_LOCK; break;
                case 'C': modifiers ^= MOD_CAPS_SHIFT; break;
                case 'S': modifiers ^= MOD_SYMBOL_SHIFT; break;
                case KEY_DEBUG: debug_level = !debug_level; break;
                case KEY_SOUND: audioSystem.setSpeakerPower(!audioSystem.isSpeakerPowered()); break;
                default: break;
                }
                redraw();
            }
            else if (!k.is_sticky && k.base_val != KEY_LAYOUT_SWAP) {
                char outChar = (modifiers & MOD_CAPS_SHIFT) ? k.shift_val : k.base_val;
                if (onKeyEvent) {
                    onKeyEvent(outChar, modifiers, isPressed);
                }
            }
            break;
        }
    }
}
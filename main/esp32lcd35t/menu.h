/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <vector>
#include <string>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "audio.h"
#include "display.h"
#include "spectrum/zx48k.h"
#include "ulist.h"
#include "uwidget.h"

#define UI_BTN_CANCEL  -1
#define UI_BTN_BACK    -1
#define UI_BTN_OK       1
#define UI_IDLE        -2
#define UI_SCREENSHOT  -3

class Menu
{
protected:
    lgfx::LGFX_Device &lcd;
    DisplaySystem &display;
    AudioSystem &audio;
    ZXSpectrum &spectrum;
    bool active;
    std::vector<ListItem> _items;
    uList *ulist;
    UIWidget *modal;
    uint16_t selectedItem = 0;
    const char *filePath;

    void openModal(UIWidget *widget);
    void closeModal();
    virtual Menu* onModalResult(int16_t result, bool &isIdle);

public:
    Menu(lgfx::LGFX_Device &_lcd, DisplaySystem &_display, AudioSystem &_audio, ZXSpectrum &_spectrum);
    virtual ~Menu();

    virtual void init() {};
    virtual void init(const char * path) {};
    virtual Menu* action(int16_t selectedIndex, bool &isIdle) = 0;

    /**
     * @brief Instantiates (if needed) and draws the active modal or uList sprite.
     */
    void draw();

    /**
     * @brief Processes touch coordinates non-blockingly.
     * @return Selected item index, -2 for page change / modal redraw, or -1 on miss.
     */
    int16_t handleTouch(int16_t touchX, int16_t touchY);

    inline bool isActive() const { return active; }
    inline void setActive(bool state) { active = state; }
    inline const LGFX_Sprite* getOverlaySprite() const {
        if (modal) return modal->getSprite();
        return ulist ? ulist->getSprite() : nullptr;
    }
};

class MainMenu : public Menu
{
private:
    const uint8_t TapeList = 0;
    const uint8_t SnaphotList = 1;
    const uint8_t MusicPlayer = 2;
    const uint8_t Settings = 3;
    const uint8_t About = 4;
    const uint8_t Exit = 5;
    const char *tapePath = "/sdcard/zx/tapes";
    const char *snapshotPath = "/sdcard/zx/snapshots";
    const char *musicPath = "/sdcard/zx/music";

public:
    MainMenu(lgfx::LGFX_Device &_lcd, DisplaySystem &_display, AudioSystem &_audio, ZXSpectrum &_spectrum);
    void init() override;
    Menu* action(int16_t selectedIndex, bool &isIdle) override;
};

class SettingsMenu : public Menu
{
protected:
    Menu* onModalResult(int16_t result, bool &isIdle) override;
    const uint8_t TapeOriginal = 0;
    const uint8_t TapeFlash = 1;
    const uint8_t Volume = 2;
    const uint8_t Brightness = 3;
    const uint8_t PressDuration = 4;
    const uint8_t ReleaseDuration = 5;

public:
    SettingsMenu(lgfx::LGFX_Device &_lcd, DisplaySystem &_display, AudioSystem &_audio, ZXSpectrum &_spectrum);
    void init() override;
    Menu* action(int16_t selectedIndex, bool &isIdle) override;
};

class FileListMenu : public Menu
{
private:
    std::vector<std::string> romFiles;
    bool noFilesFound = false;

protected:
    Menu* onModalResult(int16_t result, bool &isIdle) override;

public:
    FileListMenu(lgfx::LGFX_Device &_lcd, DisplaySystem &_display, AudioSystem &_audio, ZXSpectrum &_spectrum);
    void init() override;
    void init(const char *path) override;
    Menu* action(int16_t selectedIndex, bool &isIdle) override;
};

#endif // MENU_H
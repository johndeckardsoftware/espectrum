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
#include "../spectrum/zx48k.h"
#include "ulist.h"

class Menu
{
protected:
    lgfx::LGFX_Device &lcd;
    ZXSpectrum &spectrum;
    bool active;
    std::vector<ListItem> _items;
    uList *ulist;

public:
    Menu(lgfx::LGFX_Device &display, ZXSpectrum &_spectrum);
    virtual ~Menu();

    virtual void init() = 0;
    virtual Menu* action(int16_t selectedIndex, bool &isIdle) = 0;

    /**
     * @brief Instantiates (if needed) and draws the uList sprite.
     */
    void draw();

    /**
     * @brief Processes touch coordinates non-blockingly.
     * @return Selected item index, -2 for page change, or -1 on miss.
     */
    int16_t handleTouch(int16_t touchX, int16_t touchY);

    inline bool isActive() const { return active; }
    inline void setActive(bool state) { active = state; }
    inline const LGFX_Sprite* getOverlaySprite() const { return ulist ? ulist->getSprite() : nullptr; }
};

class MainMenu : public Menu
{
public:
    MainMenu(lgfx::LGFX_Device &display, ZXSpectrum &spectrum);
    void init() override;
    Menu* action(int16_t selectedIndex, bool &isIdle) override;
};

class SettingsMenu : public Menu
{
public:
    SettingsMenu(lgfx::LGFX_Device &display, ZXSpectrum &spectrum);
    void init() override;
    Menu* action(int16_t selectedIndex, bool &isIdle) override;
};

class FileListMenu : public Menu
{
private:
    std::vector<std::string> romFiles;

public:
    FileListMenu(lgfx::LGFX_Device &display, ZXSpectrum &spectrum);
    void init() override;
    Menu* action(int16_t selectedIndex, bool &isIdle) override;
};

#endif // MENU_H

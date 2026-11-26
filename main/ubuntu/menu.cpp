/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include "menu.h"
#include "ulist.h"
#include "../spectrum/z80_loader.h"
#include "../spectrum/sna_loader.h"
#include "platform_log.h"
#include <dirent.h>
#include <nvs_flash.h>
#include <nvs.h>

static const char *TAG = "MENU";

Menu::Menu(lgfx::LGFX_Device &display, ZXSpectrum &_spectrum) 
    : lcd(display),
      spectrum(_spectrum),
      active(true),
      ulist(nullptr)
{
}

Menu::~Menu()
{
    if (ulist) {
        delete ulist;
        ulist = nullptr;
    }
}

void Menu::draw()
{
    if (!ulist) {
        ulist = new uList(lcd, _items, spectrum.config);
    }
    ulist->drawItemList();
}

int16_t Menu::handleTouch(int16_t touchX, int16_t touchY)
{
    if (!ulist) return -1;
    // Offset coordinates relative to sprite push position
    int16_t relX = touchX - ULIST_POS_X;
    int16_t relY = touchY - ULIST_POS_Y;
    return ulist->checkKeyHit(relX, relY);
}


// --- MainMenu Implementation ---
//
MainMenu::MainMenu(lgfx::LGFX_Device &display, ZXSpectrum &spectrum) 
    : Menu(display, spectrum)
{
}

void MainMenu::init()
{
    _items = {
        "Load Game",
        "Load Music",
        "Settings",
        "About",
        "Back"
    };
}

Menu* MainMenu::action(int16_t selectedIndex, bool &isIdle)
{
    if (selectedIndex == -2) { 
        isIdle = true;
        return nullptr;
    }
    else if (selectedIndex == -1)
        return nullptr;
    else if (selectedIndex == 0) {
        Menu* gameMenu = new FileListMenu(lcd, spectrum);
        return gameMenu;
    }
    else if (selectedIndex == 1) {
//        Menu* musicMenu = new MusicListMenu(lcd, spectrum);
//        musicMenu->init();
//        return musicMenu;
    }
    else if (selectedIndex == 2) {
        Menu* settingsMenu = new SettingsMenu(lcd, spectrum);
        settingsMenu->init();
        return settingsMenu;
    }
    else if (selectedIndex == 3) {
//        Menu* settingsMenu = new SettingsMenu(lcd, spectrum);
//        settingsMenu->init();
//        return settingsMenu;
    }
    else if (selectedIndex == 4) {
        return nullptr;
    }

    return nullptr;
}


// --- SettingsMenu Implementation ---
//
SettingsMenu::SettingsMenu(lgfx::LGFX_Device &display, ZXSpectrum &spectrum) 
    : Menu(display, spectrum)
{
}

void SettingsMenu::init()
{
    _items = {
        {"Tape Load Mode Original", "tape", "original"},
        {"Tape Load Mode Fast", "tape", "fast"}
    };
}

Menu* SettingsMenu::action(int16_t selectedIndex, bool &isIdle)
{
    if (selectedIndex == -2) { 
        isIdle = true;
        return nullptr;
    }
    else if (selectedIndex == -1) {
        Menu* mainMenu = new MainMenu(lcd, spectrum);
        mainMenu->init();
        return mainMenu;
    }
    else if (selectedIndex >= 0 && static_cast<size_t>(selectedIndex) < _items.size()) {
        const ListItem& item = ulist->getItem(selectedIndex);
        if (item.hasNvs) {
            spectrum.config.set(item.nvsKey, item.nvsValue);
        }
    }

    return nullptr;
}


// --- GameListMenu Implementation ---
//
FileListMenu::FileListMenu(lgfx::LGFX_Device &display, ZXSpectrum &spectrum) 
    : Menu(display, spectrum)
{
}

void FileListMenu::init()
{
    romFiles.clear();
    DIR *dir = opendir("/sdcard/zxgames");
    if (dir != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (strstr(ent->d_name, ".Z80") != nullptr ||
                strstr(ent->d_name, ".SNA") != nullptr ||
                strstr(ent->d_name, ".TZX") != nullptr ||
                strstr(ent->d_name, ".TAP") != nullptr ) {
                romFiles.push_back(std::string(ent->d_name));
            }
        }
        closedir(dir);
    }

    if (romFiles.empty()) {
        romFiles.push_back("Z80, SNA, TAP, TZX files found.");
    }

    _items.clear();
    for (const auto& file : romFiles) {
        _items.push_back(ListItem(file));
    }
}

Menu* FileListMenu::action(int16_t selectedIndex, bool &isIdle)
{
    if (selectedIndex == -2) {
        isIdle = true;
        return nullptr;
    }
    else if (selectedIndex == -1) {
        Menu* mainMenu = new MainMenu(lcd, spectrum);
        mainMenu->init();
        return mainMenu;
    }
    else if (selectedIndex >= 0 && static_cast<size_t>(selectedIndex) < romFiles.size()) {
        std::string filename = romFiles[selectedIndex];
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "/sdcard/zxgames/%s", filename.c_str());
        if (filename.ends_with(".Z80")) {
            Z80Loader loader;
            loader.loadZ80(fullPath, &spectrum);
/*/
            SP48::Z80Snapshot snap("manic_miner.z80", true);
            if (snap.validFile) {
                snap.open(&cpu, &mmu, &outPort, nullptr, &tStates);
            }
*/
        } else if (filename.ends_with(".SNA")) {
            loadSNAFile(fullPath, &spectrum);
        } else if (filename.ends_with(".TAP") ||
                   filename.ends_with(".TZX")) {
            bool ret = spectrum.tm.OpenTape(fullPath);
            if (ret) {
                spectrum.flashLoader = spectrum.config.isTapeLoadModeFlash();
                spectrum.trapTapeLoad();
                //spectrum.sendKeys("J\"\"\r");
            }
        }
    }

    return nullptr;
}
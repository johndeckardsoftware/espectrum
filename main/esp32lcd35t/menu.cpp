/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include "menu.h"
#include "ulist.h"
#include "umsgbox.h"
#include "unumsel.h"
#include "spectrum/snaps.h"
#include "spectrum/48k_z80_tape_loader.h"
#include "spectrum/vt2esp32.h"
#include "common/helper.h"
#include "audio.h"
#include "display.h"
#include "esp_log.h"
#include <dirent.h>
#include <nvs_flash.h>
#include <nvs.h>

//static const char *TAG = "menu";

Menu::Menu(lgfx::LGFX_Device &_lcd, DisplaySystem &_display, AudioSystem &_audio, ZXSpectrum &_spectrum)
    : lcd(_lcd),
      display(_display),
      audio(_audio),
      spectrum(_spectrum),
      active(true),
      ulist(nullptr),
      modal(nullptr),
      selectedItem(0)
{
}

Menu::~Menu()
{
    closeModal();
    if (ulist) {
        delete ulist;
        ulist = nullptr;
    }
}

void Menu::openModal(UIWidget *widget)
{
    closeModal();
    modal = widget;
    if (modal) {
        modal->draw();
    }
}

void Menu::closeModal()
{
    if (modal) {
        delete modal;
        modal = nullptr;
    }
}

// 1 OK button, -1 Cancel button
Menu* Menu::onModalResult(int16_t result, bool &isIdle)
{
    (void)result;
    (void)isIdle;
    _print("base menu", "onModalResult result: %d, isIdle: %d\n", result, isIdle);
    // Redraw underlying list after closing a modal
    if (ulist) {
        ulist->drawItemList();
    }
    return nullptr;
}

void Menu::draw()
{
    if (!ulist) {
        ulist = new uList(lcd, _items, spectrum.config);
    }
    ulist->drawItemList();

    // Draw modal on top of the list so it stays visible across full draws
    if (modal) {
        modal->draw();
    }
}

int16_t Menu::handleTouch(int16_t touchX, int16_t touchY)
{
    if (modal) {
        int16_t res = modal->handleTouch(touchX, touchY);
        if (res == -2) {
            return -2; // Still interacting with modal
        }
        closeModal();
        bool dummyIdle = false;
        onModalResult(res, dummyIdle);
        // Return -2 so the outer loop does not treat modal dismissal as a list row click
        return -2;
    }

    if (!ulist) return -1;
    // Offset coordinates relative to sprite push position
    int16_t relX = touchX - ULIST_POS_X;
    int16_t relY = touchY - ULIST_POS_Y;
    return ulist->checkKeyHit(relX, relY);
}


// --- MainMenu Implementation ---

MainMenu::MainMenu(lgfx::LGFX_Device &_lcd, DisplaySystem &_display, AudioSystem &_audio, ZXSpectrum &_spectrum)
    : Menu(_lcd, _display, _audio, _spectrum)
{
    filePath = tapePath;
}

void MainMenu::init()
{
    _items = {
        "Load Tape",
        "Load Snapshot",
        "Play Music",
        "Settings",
        "About",
        "Back"
    };
}

Menu* MainMenu::action(int16_t selectedIndex, bool &isIdle)
{
    if (selectedIndex == UI_IDLE) {
        isIdle = true;
        return nullptr;
    }
    else if (selectedIndex == UI_BTN_CANCEL) {
        return nullptr;
    }
    else if (selectedIndex == TapeList) {
        auto* fileListMenu = new FileListMenu(lcd, display, audio, spectrum);
        fileListMenu->init(tapePath);
        return fileListMenu;
    }
    else if (selectedIndex == SnaphotList) {
        auto* fileListMenu = new FileListMenu(lcd, display, audio, spectrum);
        fileListMenu->init(snapshotPath);
        return fileListMenu;
    }
    else if (selectedIndex == MusicPlayer) {
        auto* fileListMenu = new FileListMenu(lcd, display, audio, spectrum);
        fileListMenu->init(musicPath);
        return fileListMenu;
    }
    else if (selectedIndex == Settings) {
        Menu* settingsMenu = new SettingsMenu(lcd, display, audio, spectrum);
        settingsMenu->init();
        return settingsMenu;
    }
    else if (selectedIndex == About) {
        openModal(new MessageBox(lcd, "About", "ZX Spectrum Emulator\n   ESP32 + LovyanGFX", true));
        isIdle = true;
    }
    else if (selectedIndex == Exit) {
        return nullptr;
    }

    return nullptr;
}


// --- SettingsMenu Implementation ---

SettingsMenu::SettingsMenu(lgfx::LGFX_Device &_lcd, DisplaySystem &_display, AudioSystem &_audio, ZXSpectrum &_spectrum)
    : Menu(_lcd, _display, _audio, _spectrum)
{
}

void SettingsMenu::init()
{
    _items = {
        {"Tape Load Mode Original", "tape", "original"},
        {"Tape Load Mode Fast", "tape", "fast"},
        {"Volume", "volume", false},
        {"Brightness", "brightness", false},
        {"Press Hold Duration", "keyPHD", false},
        {"Press Release Duration", "keyRHD", false},
    };
}

Menu* SettingsMenu::onModalResult(int16_t result, bool &isIdle)
{
    (void)isIdle;
    if (result >= 0) {
        _print("settings", "onModalResult result: %d, isIdle: %d\n", result, isIdle);
        if (selectedItem == Volume) {
            _print("settings", "onModalResult setVolume: %d\n", result);
            audio.setSpeakerVolume(result);
        }
        else if (selectedItem == Brightness) {
            _print("settings", "onModalResult setBrightness: %d\n", result);
            display.setBacklight(static_cast<uint8_t>(result));
        }
        else if (selectedItem == PressDuration) {
            _print("settings", "onModalResult pressDuration: %d\n", result);
        }
        else if (selectedItem == ReleaseDuration) {
            _print("settings", "onModalResult releaseDuration: %d\n", result);
        }
    }
    if (ulist) {
        ulist->drawItemList();
    }
    return nullptr;
}

Menu* SettingsMenu::action(int16_t selectedIndex, bool &isIdle)
{
    if (selectedIndex == UI_IDLE) {
        isIdle = true;
        return nullptr;
    }
    else if (selectedIndex == UI_BTN_BACK) {
        Menu* mainMenu = new MainMenu(lcd, display, audio, spectrum);
        mainMenu->init();
        return mainMenu;
    }
    else if (selectedIndex >= TapeOriginal && selectedIndex <= TapeFlash) {
        const ListItem& item = ulist->getItem(selectedIndex);
        if (item.hasNvs) {
            spectrum.config.set(item.nvsKey, item.nvsValue);
        }
        isIdle = true;
    }
    else if (selectedIndex == Volume) {
        if (!modal) {
            selectedItem = selectedIndex;
            const ListItem& item = ulist->getItem(selectedIndex);
            openModal(new NumberSelector(lcd, item.text, 50, 0, 90, 2, spectrum.config, item.nvsKey));
            isIdle = true;
        }
    }
    else if (selectedIndex == Brightness) {
        if (!modal) {
            selectedItem = selectedIndex;
            const ListItem& item = ulist->getItem(selectedIndex);
            openModal(new NumberSelector(lcd, item.text, 60, 0, 100, 2, spectrum.config, item.nvsKey));
            isIdle = true;
        }
    }
    else if (selectedIndex == PressDuration) {
        if (!modal) {
            selectedItem = selectedIndex;
            const ListItem& item = ulist->getItem(selectedIndex);
            openModal(new NumberSelector(lcd, item.text, 26, 0, 128, 1, spectrum.config, item.nvsKey));
            isIdle = true;
        }
    }
    else if (selectedIndex == ReleaseDuration) {
        if (!modal) {
            selectedItem = selectedIndex;
            const ListItem& item = ulist->getItem(selectedIndex);
            openModal(new NumberSelector(lcd, item.text, 19, 0, 128, 1, spectrum.config, item.nvsKey));
            isIdle = true;
        }
    }

    return nullptr;
}


// --- FileListMenu Implementation ---

FileListMenu::FileListMenu(lgfx::LGFX_Device &_lcd, DisplaySystem &_display, AudioSystem &_audio, ZXSpectrum &_spectrum)
    : Menu(_lcd, _display, _audio, _spectrum)
{
}

void FileListMenu::init()
{
    init(filePath);
}

void FileListMenu::init(const char *path)
{
    filePath = path;
    romFiles.clear();
    noFilesFound = false;
    DIR *dir = opendir(filePath);
    if (dir != NULL) {
        char filename[256] = {0};
        struct dirent *ent;
        while ((ent = readdir(dir)) != nullptr) {
            strcpy(filename, ent->d_name);
            strupr(filename);
            if (strstr(filename, ".Z80") != nullptr ||
                strstr(filename, ".SNA") != nullptr ||
                strstr(filename, ".SZX") != nullptr ||
                strstr(filename, ".TZX") != nullptr ||
                strstr(filename, ".TAP") != nullptr ||
                strstr(filename, ".PT3") != nullptr ) {
                romFiles.push_back(std::string(ent->d_name));
            }
        }
        closedir(dir);
    }

    if (romFiles.empty()) {
        noFilesFound = true;
        openModal(new MessageBox(lcd, "SD Card", "No games found in\n  /sdcard/zxgames", false));
    }

    _items.clear();
    for (const auto& file : romFiles) {
        _items.push_back(ListItem(file));
    }
}

Menu* FileListMenu::onModalResult(int16_t result, bool &isIdle)
{
    (void)result;
    (void)isIdle;
    if (ulist) {
        ulist->drawItemList();
    }
    return nullptr;
}

Menu* FileListMenu::action(int16_t selectedIndex, bool &isIdle)
{
    if (selectedIndex == -2) {
        isIdle = true;
        return nullptr;
    }
    else if (selectedIndex == -1) {
        Menu* mainMenu = new MainMenu(lcd, display, audio, spectrum);
        mainMenu->init();
        return mainMenu;
    }
    else if (selectedIndex >= 0 && static_cast<size_t>(selectedIndex) < romFiles.size()) {
        std::string name = romFiles[selectedIndex];
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", filePath, name.c_str());
        std::string ext;
        const size_t dotPos = name.find_last_of('.');
        if (dotPos != std::string::npos) {
            ext = name.substr(dotPos);
            for (char& c : ext) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
        }        
        if (ext == ".Z80" || ext == (".SNA") || ext == ".SZX") {
            std::string error;
            Snaps snaps(&spectrum);
            bool ok = snaps.loadSnapshot(fullPath, &error);
            if (!ok) {
                _print("snaps", error.c_str());
                openModal(new MessageBox(lcd, "Snapshot Error", error.empty() ? "Failed to load" : error, false));
                isIdle = true;
            }
        } else if (ext == ".TAP" ||  ext == ".TZX") {
            bool ret = spectrum.tm.OpenTape(fullPath);
            if (ret) {
                spectrum.flashLoader = spectrum.config.isTapeLoadModeFlash();
                spectrum.trapTapeLoad();
                std::string error;
                Snaps loader(&spectrum);
                loader.loadData(z80_tape_loader, sizeof(z80_tape_loader));
                if (!loader.parseZ80File(&error)) {
                    std::fprintf(stderr, "Failed to load autoload snapshot (%s)\n", error.c_str());
                }
            } else {
                openModal(new MessageBox(lcd, "Tape Error", "Could not open tape file.", false));
                isIdle = true;
            }
        } else if (ext == ".PT3") {
            // Reads .pt3. No error checks !!
            FILE* f = std::fopen(fullPath, "rb");
            std::fseek(f, 0, SEEK_END);
            size_t fileSize = std::ftell(f);
            std::fseek(f, 0, SEEK_SET);
            std::vector<uint8_t> pt3Content;
            pt3Content.resize(static_cast<size_t>(fileSize));
            size_t bytesRead = std::fread(pt3Content.data(), 1, pt3Content.size(), f);
            std::fclose(f);
            printf("filename: %s, fileSize: %d, bytesRead: %d\n", fullPath, fileSize, bytesRead);

            std::string error;
            Snaps loader(&spectrum);
            loader.loadData(vt2esp32_z80, sizeof(vt2esp32_z80));
            if (!loader.parseZ80File(&error)) {
                std::fprintf(stderr, "Failed to load vt2esp32 snapshot (%s)\n", error.c_str());
            }
            // _pt3buffer_size = $9A01 ; addr, public, , vt2play_c, data_compiler, vt2play.c:20
            // _pt3buffer = $9A03 ; addr, public, , vt2play_c, data_compiler, vt2play.c:21
            uint16_t _pt3buffer_size = spectrum.mem.read(0x9A01) + (spectrum.mem.read(0x9A02) << 8);
            for (int i = 0; i < _pt3buffer_size; i++) {spectrum.mem.write(0x9A03+i, pt3Content[i]);}
        }
    }

    return nullptr;
}
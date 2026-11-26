/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */
/****
 *
 * Main to run emulator on ubuntu with raylib
 *
 ****/
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include <raylib.h>

#include "audio.h"
#include "display.h"
#include "../spectrum/snaps.h"
#include "../spectrum/zx48k.h"
#include "../spectrum/z80.hpp"
#include "../spectrum/vt2esp32.h"

int16_t debug_level = 0;
uint32_t full_frame_time_us = 0;
uint32_t emu_frame_time_us = 0;
uint32_t lcd_push_time_us = 0;
uint32_t lcd_render_time_us = 0;
uint64_t mean_frame_time_us = 0;
int32_t full_audio_time_us = 0;
int32_t write_audio_time_us = 0;

namespace
{
struct KeyBinding
{
    int raylibKey;
    SpecKeys spectrumKey;
};

constexpr std::array<KeyBinding, 42> keyBindings{{
    {KEY_ONE, SPECKEY_1}, {KEY_TWO, SPECKEY_2}, {KEY_THREE, SPECKEY_3},
    {KEY_FOUR, SPECKEY_4}, {KEY_FIVE, SPECKEY_5}, {KEY_SIX, SPECKEY_6},
    {KEY_SEVEN, SPECKEY_7}, {KEY_EIGHT, SPECKEY_8}, {KEY_NINE, SPECKEY_9},
    {KEY_ZERO, SPECKEY_0}, {KEY_Q, SPECKEY_Q}, {KEY_W, SPECKEY_W},
    {KEY_E, SPECKEY_E}, {KEY_R, SPECKEY_R}, {KEY_T, SPECKEY_T},
    {KEY_Y, SPECKEY_Y}, {KEY_U, SPECKEY_U}, {KEY_I, SPECKEY_I},
    {KEY_O, SPECKEY_O}, {KEY_P, SPECKEY_P}, {KEY_A, SPECKEY_A},
    {KEY_S, SPECKEY_S}, {KEY_D, SPECKEY_D}, {KEY_F, SPECKEY_F},
    {KEY_G, SPECKEY_G}, {KEY_H, SPECKEY_H}, {KEY_J, SPECKEY_J},
    {KEY_K, SPECKEY_K}, {KEY_L, SPECKEY_L}, {KEY_Z, SPECKEY_Z},
    {KEY_X, SPECKEY_X}, {KEY_C, SPECKEY_C}, {KEY_V, SPECKEY_V},
    {KEY_B, SPECKEY_B}, {KEY_N, SPECKEY_N}, {KEY_M, SPECKEY_M},
    {KEY_SPACE, SPECKEY_SPACE}, {KEY_ENTER, SPECKEY_ENTER},
    {KEY_LEFT_SHIFT, SPECKEY_SHIFT}, {KEY_RIGHT_SHIFT, SPECKEY_SHIFT},
    {KEY_LEFT_ALT, SPECKEY_SYMB}, {KEY_RIGHT_ALT, SPECKEY_SYMB}
}};

void updateKeyboard(ZXSpectrum &spectrum)
{
    static std::array<bool, SPECKEY_MAX_NORMAL> previous{};
    std::array<bool, SPECKEY_MAX_NORMAL> pressedKeys{};
    for (size_t i = 0; i < keyBindings.size(); ++i)
    {
        const KeyBinding &binding = keyBindings[i];
        pressedKeys[binding.spectrumKey] = pressedKeys[binding.spectrumKey] || IsKeyDown(binding.raylibKey);
    }

    if (IsKeyDown(KEY_BACKSPACE))
    {
        pressedKeys[SPECKEY_SHIFT] = true;
        pressedKeys[SPECKEY_0] = true;
    }

    for (size_t key = 0; key < pressedKeys.size(); ++key)
    {
        if (pressedKeys[key] != previous[key])
        {
            spectrum.updateKey(static_cast<SpecKeys>(key),
                pressedKeys[key] ? KEY_PRESSED : KEY_RELEASED);
            previous[key] = pressedKeys[key];
        }
    }
}

bool loadFile(ZXSpectrum &spectrum, const std::string &filename)
{
    std::string extension = std::filesystem::path(filename).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

    if (extension == ".Z80" || extension == ".SNA" || extension == ".SZX")
    {
        std::string error;
        Snaps loader(&spectrum);
        if (!loader.loadSnapshot(filename.c_str(), &error))
        {
            std::fprintf(stderr, "Failed to load snapshot %s: %s\n", filename.c_str(), error.c_str());
            return false;
        }
        return true;
    }
    if (extension == ".TAP" || extension == ".TZX")
    {
        if (!spectrum.tm.OpenTape(filename))
        {
            std::fprintf(stderr, "Failed to open tape: %s\n", filename.c_str());
            return false;
        }
        spectrum.flashLoader = spectrum.config.isTapeLoadModeFlash();
        spectrum.flashLoader = true;
        spectrum.trapTapeLoad();

        std::string error;
        Snaps loader(&spectrum);
        if (!loader.loadSnapshot("main/support/tapeloaders/Load48.z80", &error)) {
            std::fprintf(stderr, "Failed to load snapshot %s: %s\n", filename.c_str(), error.c_str());
            return false;
        }

        //char loadCommand[] = "J\"\"\r";
        //spectrum.sendKeys(loadCommand);
        return true;
    }

    std::fprintf(stderr, "Unsupported file type: %s (expected .z80, .sna, .tap, or .tzx)\n",
        filename.c_str());
    return false;
}
}

int main(int argc, char **argv)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(960, 746, "eSpectrum Desktop");
    SetTargetFPS(50);
    SetExitKey(KEY_ESCAPE);

    int result = 0;
    {
        DisplaySystem display;
        display.init();

        AudioSystem audio;
        audio.init();

        ZXSpectrum spectrum;
        if (!spectrum.initZX48K())
            return 1;

        bool runEmulator = argc <= 1 || loadFile(spectrum, argv[1]);
        if (!runEmulator)
            result = 1;

        std::array<uint16_t, BEEPER_SAMPLES_SIZE> beeperSamples{};
        bool showDebug = false;

        while (runEmulator && !WindowShouldClose())
        {
            if (IsKeyPressed(KEY_F1))
                spectrum.paused = !spectrum.paused;
            if (IsKeyPressed(KEY_F2))
            {
                if (spectrum.tm.IsTapePlaying)
                    spectrum.tm.PauseTape();
                else
                    spectrum.tm.PlayTape(true);
            }
            if (IsKeyPressed(KEY_F3))
            {
                std::string error;
                std::string filename = "/home/ubuntu/devs/esp/espectrum/build_ubuntu/snapshot/snake.z80";
                Snaps loader(&spectrum);
                if (!loader.loadSnapshot(filename, &error)) {
                    std::fprintf(stderr, "Failed to load snapshot %s: %s\n", filename.c_str(), error.c_str());
                    return false;
                }
                //spectrum.mem.dumpMemory();
                //spectrum.z80.registerDump();
                //char loadCommand[] = "J\"\"\r";
                //spectrum.sendKeys(loadCommand);
            }
            if (IsKeyPressed(KEY_F4))
            {
                std::string error;
                char * fullPath = "/home/ubuntu/devs/esp/espectrum/build_ubuntu/music/music.pt3";
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

                //std::string error;
                Snaps loader(&spectrum);
                loader.loadData(vt2esp32_z80, sizeof(vt2esp32_z80));
                if (!loader.parseZ80File(&error)) {
                    std::fprintf(stderr, "Failed to load vt2esp32 snapshot (%s)\n", error.c_str());
                }
                //_pt3buffer = $9A03 ; addr, public, , vt2play_c, data_compiler, vt2play.c:21
                printf("%02x%02x%02x\n", spectrum.mem.read(0x9A03), spectrum.mem.read(0x9A04), spectrum.mem.read(0x9A05));
                for (int i = 0; i < bytesRead; i++) {
                    spectrum.mem.write(0x9A03+i, pt3Content[i]);
                }
                printf("%02x%02x%02x\n", spectrum.mem.read(0x9A03), spectrum.mem.read(0x9A04), spectrum.mem.read(0x9A05));
            }
            if (IsKeyPressed(KEY_F5))
                showDebug = !showDebug;
            if (IsKeyPressed(KEY_F11))
                ToggleFullscreen();

            updateKeyboard(spectrum);

            const auto frameStart = std::chrono::steady_clock::now();
            if (!spectrum.paused)
            {
                const auto emulationStart = std::chrono::steady_clock::now();
                spectrum.runForFrame(beeperSamples.data());
                emu_frame_time_us = static_cast<uint32_t>(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - emulationStart).count());
                audio.pushBeeperSamples(beeperSamples.data());
            }

            display.renderSpectrumScreen(&spectrum);
            lcd_render_time_us = display.getLcdRenderTimeUs();
            full_frame_time_us = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - frameStart).count());
            mean_frame_time_us = mean_frame_time_us == 0
                ? full_frame_time_us
                : (mean_frame_time_us + full_frame_time_us) / 2;

            BeginDrawing();
            ClearBackground(BLACK);
            display.draw();
            DrawText("F1 pause  F2 tape  F3 LOAD  F5 debug  F11 fullscreen  Esc quit",
                12, 12, 18, RAYWHITE);
            if (showDebug)
            {
                DrawText(TextFormat("PC:%04X  SP:%04X  frame:%u us  render:%u us  tape:%s",
                    static_cast<unsigned>(spectrum.z80.reg.PC),
                    static_cast<unsigned>(spectrum.z80.reg.SP), full_frame_time_us,
                    lcd_render_time_us, spectrum.tm.IsTapePlaying ? "playing" : "stopped"),
                    12, 36, 18, YELLOW);
            }
            if (spectrum.paused)
                DrawText("PAUSED", 12, 62, 22, ORANGE);
            EndDrawing();
        }
    }

    CloseWindow();
    return result;
}


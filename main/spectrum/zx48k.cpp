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

#include "zx48k.h"
#include "48k_rom.h"
#include "AySound.h"
#include "keyboard.h"
#include "tape_flash.cpp"

static const char *TAG = "zx48k";

/**
 * @brief Construct a new ZXSpectrum class.
 */
ZXSpectrum::ZXSpectrum()
    : mem(type),
      z80(),
      tm(port)
{
    z80.setupCallback(
        [](void* arg, unsigned short addr) -> unsigned char {
            return static_cast<ZXSpectrum*>(arg)->mem.read(addr);
        },
        [](void* arg, unsigned short addr, unsigned char value) {
            static_cast<ZXSpectrum*>(arg)->mem.write(addr, value);
        },
        [](void* arg, unsigned short port) -> unsigned char {
            return static_cast<ZXSpectrum*>(arg)->port.in(port);
        },
        [](void* arg, unsigned short port, unsigned char value) {
            static_cast<ZXSpectrum*>(arg)->port.out(port, value);
        },
        this,
        true
    );

    z80.setMemory(&mem);
}

/**
 * @brief Load the system ROM, and verify signatures.
 * @return true if initialization and ROM signature verification succeeded, false otherwise.
 */
bool ZXSpectrum::initZX48K()
{
    flashLoader = config.isTapeLoadModeFlash();

    // Load the 16KB system ROM binary data into memory base
    mem.loadRom(ROM_ZX48K, mem.getPageSize());

    // setup the AYSound emulator
    ay.init();
    ay.set_sound_format(15625, 1, 8);
    ay.set_stereo(AYEMU_MONO, NULL);

    ay.reset();

    _print(TAG, "ZX48k started");

    return true;
}

/**
 * @brief Run the emulation loop for exactly one frame duration.
 * @details Simulates 312 video lines * 224 t-states per line = 69888 total t-states.
 * @param accumOut Optional buffer pointer to receive raw line-by-line audio states.
 * @return Total number of CPU t-states cycles actually executed.
 */
int ZXSpectrum::runForFrame(uint16_t *accumOut)
{
    static int totalFrames = 0;

    const int LINES_PER_FRAME = 312;
    const int TSTATES_PER_LINE = 224;
    int cyclesExecuted = 0;

    // Line-by-line video frame emulation loop
    for (int line = 0; line < LINES_PER_FRAME; line++)
    {
        // Reset the audio sample accumulator before executing the scanline
        soundSample = 0;

        // Run the CPU engine for the duration of one display line (224 t-states)
        int used = runForCycles(TSTATES_PER_LINE);
        cyclesExecuted += used;

        // Store the raw sound accumulation value for the current line
        if (accumOut)
        {
            accumOut[line] = soundSample;
        }

        // AY emulation
        if (port.ayUsed) {
            AySound::gen_sound(1, line);
        }

        // Combine beeper and AY sound safely
        // You can also apply weights here if one overpowers the other (e.g., AySound::SamplebufAY[line] * 2)
        accumOut[line] = accumOut[line] + AySound::SamplebufAY[line] * 2;
    }

    // Trigger standard vertical retrace maskable interrupt at the end of the frame
    if (z80.reg.IFF & 0x01)
    {
        switch (z80.reg.interrupt & 0b11)
        {
            case 0: // Spectrum ULA's IM 0 response is RST 38h.
                z80.generateIRQ(7);
                break;
            case 1:
                z80.generateIRQ(0x38);
                break;
            case 2: // Spectrum ULA's floating data bus reads as 0xFF.
                z80.generateIRQ(0xFF);
                break;
        }
    }

    totalFrames++;
    return cyclesExecuted;
}

/**
 * @brief Update the hardware keyboard matrix state based on physical key inputs.
 * @param key The specific spectrum key enum to update.
 * @param state Input state (1 = Pressed, 0 = Released).
 */
void ZXSpectrum::updateKey(SpecKeys key, uint8_t state)
{
    keyboard.updateKey(key, state);
}

/**
 * @brief Emulate a key press in the hardware keyboard matrix state based on key input.
 * @param key The specific spectrum key enum to press.
 */
void ZXSpectrum::pressKey(SpecKeys key)
{
    keyboard.updateKey(key, KEY_PRESSED);
    keyboard.updateKey(key, KEY_RELEASED);
}

/**
 * @brief Convert a text string in equivalent key strokes.
 * @param text The text to send to the keyboard.
 */
void ZXSpectrum::sendKeys(const char * text) {
    int l = strlen(text);
    for (int i = 0; i < l; i++) {
        char c = text[i];
        auto it = letterToSpecKeys.find(c);
        if (it != letterToSpecKeys.end())
        {
            size_t keyCount = it->second.size();
            if (keyCount == 1) {
                pressKey(it->second[0]);
            } else if (keyCount == 2) { // one modifier
                keyboard.updateKey(it->second[0], KEY_PRESSED);
                pressKey(it->second[1]);
                keyboard.updateKey(it->second[0], KEY_RELEASED);
            } else if (keyCount == 3) { // two modifier
                keyboard.updateKey(it->second[0], KEY_PRESSED);
                keyboard.updateKey(it->second[1], KEY_PRESSED);
                pressKey(it->second[2]);
                keyboard.updateKey(it->second[1], KEY_RELEASED);
                keyboard.updateKey(it->second[0], KEY_RELEASED);
            }
        }
    }
}

// Tape Emulation
/**
 * @brief Emulate mic input
 * @param pc not used
 * @param op_tstates t_states of the current z80 operand
 */
void ZXSpectrum::tapePlaying(uint16_t pc, int op_tstates)
{
    if (tm.IsTapePlaying)
    {
        if (pc == 0x056b && flashLoader)
        {
            TapeFlashLoader *tfl = new TapeFlashLoader();
            int tape_index = tfl->FlashLoad(z80, mem, tm);
            if (tape_index >= 0)
            {
                _lastReadTapeIndex = tape_index;
            }
            tm.ResetEarInput();
        }
        else
        {
            tm.ProcessTapeData(op_tstates);
        }
    }
    else
    {
        z80.resetConsumeClockCallback();
        tm.ResetEarInput();
    }
}

/**
 * @brief Callback for load tape rom routines being executed.
 */
void ZXSpectrum::tapeLoaded()
{
    z80.removeAllBreakPoints();

    if (!tm.IsTapeEmpty() && !tm.IsOpening) {
        z80.setConsumeClockCallback([](void* arg, int clocks) {
            auto* spectrum = static_cast<ZXSpectrum*>(arg);
            spectrum->tapePlaying(spectrum->z80.reg.PC, clocks);
        });

        tm.PlayTape(true);
    }
}

/**
 * @brief To be called after a sucessful TapeManager.OpenTape.
 * Setup the trap for load tape rom routines
 */
void ZXSpectrum::trapTapeLoad()
{
    z80.addBreakPoint(0x056b, [](void* arg) -> void {
        auto* spectrum = static_cast<ZXSpectrum*>(arg);
        spectrum->tapeLoaded();
    });
}



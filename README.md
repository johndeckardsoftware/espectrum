# eSpectrum

eSpectrum is a ZX Spectrum 48K emulator for the **ESP32-S3-Touch-LCD-3.5
Rev2.0** board, with a desktop Linux build for development and testing.

The project includes Z80 CPU emulation, Spectrum display rendering, beeper and
AY audio, touchscreen controls, virtual keyboard layouts, tape loading,
snapshots, screenshots, and USB mass-storage access to the SD card.

## Hardware

- Espressif ESP32-S3
- Waveshare ESP32-S3-Touch-LCD-3.5 Rev2.0
- 3.5-inch 480 x 320 LCD
- Capacitive touchscreen
- SD card
- ES8311 audio codec
- TCA9554 I/O expander

The ESP-IDF configuration in this repository targets the ESP32-S3 with 16 MB
flash, 8 MB PSRAM, a 240 MHz CPU, and an 80 MHz octal PSRAM clock.

## Features

- ZX Spectrum 48K emulation
- Z80 execution based on the SUZUKI PLAN Z80 emulator
- TAP and TZX tape loading
- Z80, SNA, and SZX snapshot loading
- Original and fast tape-load modes
- Beeper and AY sound emulation
- Touchscreen virtual keyboard
- QWERTY, gamepad, and configurable keyboard layouts
- Spectrum and full-screen BMP screenshots
- SD card file browser
- USB mass-storage mode for transferring files
- Desktop build using raylib

## SD card layout

Place files on the SD card in these directories:

```text
/sdcard/zx/tapes/
/sdcard/zx/snapshots/
/sdcard/zx/music/
```

Open **Menu** on the virtual keyboard to browse tapes, snapshots, or music
from the corresponding directory.

To expose the SD card over USB, select **USB** in the virtual keyboard layout.
The board reboots into USB mass-storage mode. Tap **REBOOT** on the screen to
return to the emulator.

## ESP32-S3 build

### Requirements

- ESP-IDF 5.x
- ESP32-S3 board support
- A USB data cable

Set up ESP-IDF, then build and flash:

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Replace `/dev/ttyACM0` with your board's serial port if necessary. On Linux,
see [SERIAL_PORT_TROUBLESHOOTING.md](SERIAL_PORT_TROUBLESHOOTING.md) for device
detection, permissions, and download-mode help.

You can also use the existing build and flash tasks in VS Code with the
Espressif IDF extension.

## Desktop build

A Linux desktop build is included for emulator development. It uses raylib,
which is already present in `build_ubuntu/raylib`.

Install the native build dependencies used by the Makefile. On Debian or
Ubuntu:

```sh
sudo apt install build-essential libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libgl1-mesa-dev zlib1g-dev
```

Build and run:

```sh
make -C build_ubuntu
./build_ubuntu/espectrum-ubuntu
```

You can load a snapshot or tape from the command line:

```sh
./build_ubuntu/espectrum-ubuntu build_ubuntu/snapshot/snake.z80
./build_ubuntu/espectrum-ubuntu build_ubuntu/tapes/snake.tap
```

Desktop keyboard controls:

| Key | Action |
| --- | --- |
| F1 | Pause/resume |
| F2 | Play/pause tape |
| F3 | Load the included example snapshot |
| F4 | Load the included PT3 music test |
| F5 | Toggle debug information |
| F11 | Toggle fullscreen |
| Esc | Quit |
| Alt | Symbol Shift |

Regular keys and Shift are mapped to the Spectrum keyboard.

To clean the desktop build:

```sh
make -C build_ubuntu clean
```

## Project structure

```text
main/
  main.cpp                 ESP32-S3 application entry point
  common/                  Shared helpers
  esp32lcd35t/             Board, display, menu, audio, and touch code
  spectrum/                Spectrum, Z80, tape, snapshot, and AY emulation
  ubuntu/                  raylib desktop implementation
components/                Board and peripheral components
build_ubuntu/              Desktop build output and example files
doc/                       Datasheets and format documentation
managed_components/        ESP-IDF managed dependencies
```

## Credits

This project is adapted from:

- [atomic14](https://github.com/atomic14/esp32-zxspectrum)
- [SUZUKI PLAN Z80 emulator](https://github.com/suzukiplan/z80)
- SoftSpectrum48 snapshot and tape handling
- Sashnov Alexander <sashnov@ngs.ru> and Roman Scherbakov <v_soft@nm.ru>
- LovyanGFX
- Espressif ESP-IDF components

ZX Spectrum is a trademark of its respective owner. This project is not
affiliated with or endorsed by Sinclair Research or its successors.

## License

No repository-level license file is currently present. Source headers indicate
that the original implementation is GPL-3.0-licensed, while third-party
components retain their own licenses. Review the headers and bundled component
licenses before distributing a binary or derivative work.

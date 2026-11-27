/*
 *  _____ ____  ____            _
 *  | ____/ ___||  _ \ ___  ___ | |_ _   _ _ __ ___
 *  |  _| \___ \| |_) / _ \/ __|| __| | | | '_ ` _ \
 *  | |___ ___) |  __/  __/ (__ | |_| |_| | | | | | |
 *  |_____|____/|_|   \___|\___| \__|\__,_|_| |_| |_| 
 *
 *  Spectrum for ESP32
 *  
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */
#include <stdio.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_task_wdt.h"
#include "driver/i2c_master.h"
#include "esp_io_expander_tca9554.h"
#include "tinyusb_default_config.h"
#include "tinyusb.h"
#include "tinyusb_msc.h"
#include "sdmmc_cmd.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"

// Hardware driver dependencies port interfaces
#include "esp_axp2101_port.h"
#include "esp_camera_port.h"
#include "esp_es8311_port.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_pcf85063_port.h"
#include "esp_qmi8658_port.h"
#include "esp_sdcard_port.h"
#include "esp_wifi_port.h"
#include "esp_lcd_touch_ft6336.h"
#include "esp_3inch5_lcd_port.h"
#include "esp_codec_dev.h"

// Subsystem Modules Class Components
#include "spectrum/zx48k.h"
#include "esp32lcd35t/display.h"
#include "esp32lcd35t/audio.h"
#include "esp32lcd35t/virtual_keyboard.h"
#include "esp32lcd35t/menu.h"

// Hardware I/O mapping configurations
#define I2C_PORT_NUM 0
#define PIN_I2C_SDA GPIO_NUM_8
#define PIN_I2C_SCL GPIO_NUM_7

static const char *TAG = "main";

// Shared performance metrics linkages referenced globally
int16_t debug_level = 0;
uint32_t full_frame_time_us = 0;
uint32_t emu_frame_time_us = 0;
uint32_t lcd_push_time_us = 0;
uint32_t lcd_render_time_us = 0;
uint64_t mean_frame_time_us = 0;
int32_t full_audio_time_us = 0;
int32_t write_audio_time_us = 0;

// Shared global devices handle
i2c_master_bus_handle_t i2c_bus_handle;
esp_io_expander_handle_t expander_handle = nullptr;
extern esp_codec_dev_handle_t output_dev;
extern esp_codec_dev_handle_t input_dev;
esp_lcd_touch_handle_t touch_handle = nullptr;

// Instantiate the core global subsystem modules components objects
ZXSpectrum *spectrum = nullptr;
DisplaySystem displaySystem;
AudioSystem audioSystem;
VirtualKeyboard *vkb = nullptr;

// Menu state
Menu *current_menu = nullptr;
bool menu_active = false;
bool menuIsIdle = false;

// USB Mass Storage variables
#define USB_MSC_MAGIC 0x55534231 // "USB1"
extern sdmmc_card_t *card;
RTC_NOINIT_ATTR uint32_t rtc_usb_mode_flag;

void screenShot(void)
{
    int prog = Config::getScreenshotProg();
    char fullPath[256];
    snprintf(fullPath, sizeof(fullPath), "/sdcard/screenshot_%03d.bmp", prog);
    displaySystem.saveFullScreenshot(fullPath, vkb, (current_menu ? current_menu->getOverlaySprite() : nullptr));

    snprintf(fullPath, sizeof(fullPath), "/sdcard/zxscreenshot_%03d.bmp", prog);
    displaySystem.saveSpectrumScreenshot(fullPath);

    auto &lcd = displaySystem.getLcdDriver();
    lcd.setCursor(210, 102);
    lcd.setTextSize(4);
    lcd.setTextColor(TFT_GREEN, TFT_TRANSPARENT);
    lcd.print("[+]");
    vkb->redraw();
}

void vkb2zxkb(char keycode, uint8_t modifiers, bool pressed)
{
    // Apply visual sync parameters shifts patterns matching Sinclair hardware specifications
    spectrum->updateKey(SPECKEY_SHIFT, (modifiers & MOD_CAPS_SHIFT) ? KEY_PRESSED : KEY_RELEASED);
    spectrum->updateKey(SPECKEY_SYMB, (modifiers & MOD_SYMBOL_SHIFT) ? KEY_PRESSED : KEY_RELEASED);

    if ((keycode > 0x20) && (keycode < 0x7F))
    {
        auto it = letterToSpecKeys.find(keycode);
        if (it != letterToSpecKeys.end())
        {
            for (SpecKeys k : it->second)
                spectrum->updateKey(k, KEY_PRESSED);
            for (SpecKeys k : it->second)
                spectrum->updateKey(k, KEY_RELEASED);

            //printf("key '%c'\n", keycode);
            return;
        }
    }
    else if (keycode == KEY_MENU) {
        if (!menu_active) {
            menu_active = true;
            spectrum->paused = true;
            current_menu = new MainMenu(displaySystem.getLcdDriver(), displaySystem, audioSystem, *spectrum);
            current_menu->init();
            current_menu->draw(); // Draw initial frame
        } else {
            menu_active = false;
            spectrum->paused = false;
            delete current_menu;
            current_menu = nullptr;
        }
        return;
    }
    else if (keycode == KEY_RUN)
    {
        spectrum->sendKeys("R\r");
    }
    else if (keycode == KEY_LOAD)
    {
        spectrum->sendKeys("J\"\"\r");
    }
    else if (keycode == KEY_SCREENSHOT)
    {
        screenShot();
    }
    else if (keycode == KEY_TEST)
    {
    }

    // Direct mapping conversions for functional utility navigation markers
    //printf("keycode: %d\n",keycode);
    SpecKeys specKey = SPECKEY_NONE;
    if (keycode == 32)
        specKey = SPECKEY_SPACE; // Space
    else if (keycode == 10)
        specKey = SPECKEY_ENTER; // Enter
    else if (keycode == 8)
    { // Backspace -> Delete (Shift + 0)
        spectrum->updateKey(SPECKEY_SHIFT, KEY_PRESSED);
        spectrum->updateKey(SPECKEY_0, KEY_PRESSED);
        spectrum->updateKey(SPECKEY_0, KEY_RELEASED);
        spectrum->updateKey(SPECKEY_SHIFT, KEY_RELEASED);
        return;
    }

    if (specKey != SPECKEY_NONE)
    {
        spectrum->pressKey(specKey);
    }
}

void draw_performance_info()
{
    double fft_ms = full_frame_time_us * 0.001f;
    double eft_ms = emu_frame_time_us * 0.001f;
    double lpt_ms = lcd_push_time_us * 0.001f;
    double lrt_ms = lcd_render_time_us * 0.001f;
    double mft_ms = mean_frame_time_us * 0.001f;

    // Retrieve real-time metrics from the audio subsystem
    double aft_ms = audioSystem.getFullAudioTimeUs() * 0.001f;
    double wft_ms = audioSystem.getWriteAudioTimeUs() * 0.001f;

    // Calls the LovyanGFX driver managed by the displaySystem module
    auto &lcd = displaySystem.getLcdDriver();

    lcd.setTextSize(1);
    lcd.setTextColor(TFT_GREEN, TFT_TRANSPARENT);

    char text[256];

    // Frame computation time metrics and Z80 emulation
    lcd.setCursor(0, 300);
    sprintf(text, "fft:%.2fms eft:%.2fms lpt:%.2fms lrt:%.2fms mft:%.2fms     ",
            fft_ms, eft_ms, lpt_ms, lrt_ms, mft_ms);
    lcd.print(text);

    // Performance metrics for the I2S audio stream and ES8311
    lcd.setCursor(0, 312);
    sprintf(text, "peak:%d fa:%.2fms wa:%.2fms t:%d eb:%d sa:%d mo:%d mi:%d     ",
     audioSystem.getAudioPeak(), aft_ms, wft_ms,
     spectrum->tm.IsTapePlaying, spectrum->port.earBit, spectrum->soundSample, spectrum->port.micOutBit, spectrum->port.micInBit);
    lcd.print(text);
}

void emulator_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);

    uint32_t frame_count = 0;

    esp_lcd_touch_point_data_t touch_data[1] = {};
    uint16_t touchX, touchY;
    uint8_t points = 0;

    uint16_t raw_beeper_samples[BEEPER_SAMPLES_SIZE];
    spectrum = new ZXSpectrum();
    spectrum->initZX48K();
    vkb->setEmuKeyboard(&spectrum->keyboard);

    if (esp_task_wdt_add(NULL) != ESP_OK)
        ESP_LOGW(TAG, "Watchdog configuration link failed");

    while (1)
    {
        esp_task_wdt_reset();

        if (menu_active && current_menu)
        {
            esp_err_t err = esp_lcd_touch_get_data(touch_handle, touch_data, &points, 1);
            if (err == ESP_OK && points > 0)
            {
                touchX = touch_data[0].x; touchY = touch_data[0].y;
                int16_t selectedArea = current_menu->handleTouch(touchX, touchY);
                if (selectedArea == UI_SCREENSHOT) {
                    screenShot();
                    selectedArea = UI_IDLE;
                }
                Menu *next_menu = current_menu->action(selectedArea, menuIsIdle);
                printf("selectedArea=%d, menuIsIdle=%d\n",selectedArea, menuIsIdle);
                if (!menuIsIdle)
                {
                    delete current_menu;
                    current_menu = next_menu;
                    if (current_menu)
                    {
                        current_menu->init();
                        current_menu->draw();
                        menuIsIdle = false;
                    }
                    else
                    {
                        menu_active = false;
                        spectrum->paused = false;
                        vkb->redraw();
                    }
                }
                else
                {menuIsIdle = false;}
                vTaskDelay(pdMS_TO_TICKS(200)); // Debounce touch
            }
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
            continue; // Skip emulation frame while menu is open
        }
        else
        {
            uint64_t fullFrameStart = esp_timer_get_time();

            // Process raw touchscreen coordinates and forward hits down inside the engine
            esp_err_t err = esp_lcd_touch_get_data(touch_handle, touch_data, &points, 1);
            if (err == ESP_OK && points > 0)
            {
                touchX = touch_data[0].x; touchY = touch_data[0].y;
                vkb->handleTouch(touchX, touchY, true);
                if (menu_active && current_menu)
                {
                    vTaskDelayUntil(&xLastWakeTime, xFrequency);
                    continue;
                }
            }

            if (!spectrum->paused) {
                uint64_t emu_start_us = esp_timer_get_time();
                spectrum->runForFrame(raw_beeper_samples);
                emu_frame_time_us = esp_timer_get_time() - emu_start_us;

                // Render alternating interlaced intervals ensuring robust 50Hz framework timings
                if ((frame_count % 2) == 0)
                {
                    displaySystem.renderSpectrumScreen(spectrum);
                    displaySystem.pushFrameDMA();

                    lcd_render_time_us = displaySystem.getLcdRenderTimeUs();
                    lcd_push_time_us = displaySystem.getLcdPushTimeUs();
                }

                audioSystem.pushBeeperSamples(raw_beeper_samples);

                uint64_t fullFrameEnd = esp_timer_get_time() - fullFrameStart;
                mean_frame_time_us = (mean_frame_time_us + fullFrameEnd) / 2;
                if ((frame_count % 2) == 0) full_frame_time_us = fullFrameEnd;

                if (frame_count % 50 == 0)
                {
                    if (debug_level > 0)
                    {
                        draw_performance_info();
                    }
                }

                frame_count++;
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void lcd_touch_polling_task(void *arg)
{
    while (1)
    {
        esp_lcd_touch_read_data(touch_handle);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void i2c_bus_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = (i2c_port_num_t)I2C_PORT_NUM;
    i2c_mst_config.scl_io_num = PIN_I2C_SCL;
    i2c_mst_config.sda_io_num = PIN_I2C_SDA;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = 1;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));
}

void io_expander_init(void)
{
    ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca9554(i2c_bus_handle, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &expander_handle));
    ESP_ERROR_CHECK(esp_io_expander_set_dir(expander_handle, IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT));
    ESP_ERROR_CHECK(esp_io_expander_set_dir(expander_handle, IO_EXPANDER_PIN_NUM_7, IO_EXPANDER_OUTPUT));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_1, 0));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_1, 1));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_7, 1));
    vTaskDelay(pdMS_TO_TICKS(100));
}

/* Storage global variables */
tinyusb_msc_storage_handle_t storage_handle = NULL;
tinyusb_msc_mount_point_t mp;

// Start the board as a usb mass storage device
void usb_mass_storage_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);

    if (esp_task_wdt_add(NULL) != ESP_OK)
        ESP_LOGW(TAG, "Watchdog configuration link failed");

    // Setup only the minimum: Display, Touch
    i2c_bus_init();
    io_expander_init();
    esp_3inch5_touch_port_init(&touch_handle, i2c_bus_handle, 320, 480, 270);
    esp_3inch5_brightness_port_init();
    displaySystem.init();

    auto &lcd = displaySystem.getLcdDriver();
    lcd.fillScreen(TFT_BLACK);

    #define EPNUM_MSC       1
    #define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

    enum {
        ITF_NUM_MSC = 0,
        ITF_NUM_TOTAL
    };

    enum {
        EDPT_CTRL_OUT = 0x00,
        EDPT_CTRL_IN  = 0x80,

        EDPT_MSC_OUT  = 0x01,
        EDPT_MSC_IN   = 0x81,
    };

    static tusb_desc_device_t descriptor_config = {
        .bLength = sizeof(descriptor_config),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = 0x303A, // This is Espressif VID. This needs to be changed according to Users / Customers
        .idProduct = 0x4002,
        .bcdDevice = 0x100,
        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,
        .bNumConfigurations = 0x01
    };

    static uint8_t const msc_fs_configuration_desc[] = {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

        // Interface number, string index, EP Out & EP In address, EP size
        TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 64),
    };

    static char const *string_desc_arr[] = {
        (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
        "TinyUSB",                      // 1: Manufacturer
        "TinyUSB Device",               // 2: Product
        "123456",                       // 3: Serials
        "ESPectrum",                    // 4. MSC
    };

    static sdmmc_card_t *usb_card = NULL;
    ESP_ERROR_CHECK(storage_init_sdmmc(&usb_card));

    tinyusb_msc_storage_config_t storage_cfg = {
        .medium = {
            .card = usb_card,
        },
        .fat_fs = {
            .base_path = NULL,                              // Use default base path
            .config = {
                .format_if_mount_failed = false,
                .max_files = 5,                             // Maximum number of files that can be opened simultaneously
                .allocation_unit_size = 0,
                .disk_status_check_enable = false,
                .use_one_fat = false,
                .rootdir_entries = 0,
            },
            .do_not_format = false,
            .format_flags = 0,                              // No special format flags
        },
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB,       // Initial mount point to USB
    };

    ESP_ERROR_CHECK(tinyusb_msc_new_storage_sdmmc(&storage_cfg, &storage_handle));

    // Configure the callback for mount changed events
    //ESP_ERROR_CHECK(tinyusb_msc_set_storage_callback(storage_mount_changed_cb, NULL));
    // Re-mount to the APP
    //_mount();

    ESP_LOGI(TAG, "USB MSC initialization");

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    tusb_cfg.descriptor.device = &descriptor_config;
    tusb_cfg.descriptor.full_speed_config = msc_fs_configuration_desc;
    tusb_cfg.descriptor.string = string_desc_arr;
    tusb_cfg.descriptor.string_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    ESP_LOGI(TAG, "USB MSC initialization DONE");

    lcd.fillRect(0, 0, 96, 32, TFT_RED);
    lcd.setTextColor(TFT_WHITE, TFT_RED);
    lcd.setTextSize(2);
    lcd.setCursor(12, 8);
    lcd.print("REBOOT");

    lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    lcd.setCursor(20, 120);
    lcd.print("USB MASS STORAGE ACTIVE");
    lcd.setTextSize(2);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setCursor(20, 175);
    lcd.print("Touch REBOOT to return to emulator.");

    // Loop waiting to reboot
    esp_lcd_touch_point_data_t touch_data[1] = {};
    uint8_t points = 0;
    while (1) {
        esp_task_wdt_reset();

        esp_lcd_touch_read_data(touch_handle);
        if (esp_lcd_touch_get_data(touch_handle, touch_data, &points, 1) == ESP_OK && points > 0) {
            if (touch_data[0].x < 96 && touch_data[0].y < 32) {
                esp_restart(); // rtc_usb_mode_flag has already been resetted before entering the loop
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Function to enable USB MSC, called by VirtualKeyboard key "USB"
void enable_usb_mass_storage(void)
{
    rtc_usb_mode_flag = USB_MSC_MAGIC;
    esp_restart();
}

extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Controlla se e' stato richiesto l'avvio in modalita' Chiavetta USB
    if (rtc_usb_mode_flag == USB_MSC_MAGIC) {
        rtc_usb_mode_flag = 0; // Reset it now, so that the emulator returns upon the next reset
        xTaskCreatePinnedToCore(usb_mass_storage_task, "usb_mass_storage_task", 8192, NULL, 5, NULL, 1);
        return;
    }

    // Initialize Peripherals
    i2c_bus_init();
    io_expander_init();
    esp_3inch5_touch_port_init(&touch_handle, i2c_bus_handle, 320, 480, 270);
    // esp_axp2101_port_init(i2c_bus_handle);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_es8311_port_init(i2c_bus_handle);
    // esp_qmi8658_port_init(i2c_bus_handle);
    esp_pcf85063_port_init(i2c_bus_handle);
    esp_sdcard_port_init();
    // esp_camera_port_init(I2C_PORT_NUM);
    // esp_wifi_port_init("WSTEST", "waveshare0755");

    esp_3inch5_brightness_port_init();

    // Initialize lcd display and audio system
    displaySystem.init();
    audioSystem.init(expander_handle, output_dev);

    if (xTaskCreate(lcd_touch_polling_task, "lcd touch polling", 4096, NULL, 5, NULL) != pdPASS)
    {
        ESP_LOGE("TEST", "Failed to create lcd touch polling task");
    }

    vkb = new VirtualKeyboard(displaySystem.getLcdDriver(), vkb2zxkb);
    vkb->init();

    xTaskCreatePinnedToCore(emulator_task, "emulator_task", 8192, NULL, 5, NULL, 1);
}

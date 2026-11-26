/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_codec_dev.h"
#include "esp_io_expander.h"

#define AUDIO_SAMPLE_RATE    16000
#define BEEPER_SAMPLES_SIZE  312
#define BEEPER_SAMPLES_RATE  312 * 50  // 312 samples * 50 fps
#define AUDIO_SAMPLES_SIZE   (BEEPER_SAMPLES_SIZE * 2)

/**
 * @brief Structure to hold structural Biquad Filter coefficients and delay history.
 */
struct BiquadState
{
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2; 
};

/**
 * @brief Subsystem class managing ESP32-S3 audio rendering, I2S codecs, and digital filtering.
 */
class AudioSystem
{
private:
    esp_io_expander_handle_t ioExpander; ///< Hardware expander handle for PA power control.
    esp_codec_dev_handle_t outputDev;    ///< Handle for the audio output codec device.
    TaskHandle_t audioTaskHandle;        ///< FreeRTOS background stream task handle.
    
    uint16_t audioSamples[AUDIO_SAMPLES_SIZE]; ///< Internal double buffer for processing chunks.
    uint16_t audioSamplesIndex;                ///< Write pointer index for the incoming buffer.
    bool audioSamplesReady;                    ///< Flag signaling that a full block is ready to render.
    bool speakerPowered;                       ///< Monitor current state of the speaker hardware amplifier.
    uint16_t speakerVolume;

    // Telemetry and Profiling counters
    uint32_t countZeros;
    uint32_t countNzeros;
    uint16_t audioPeak;
    int32_t fullAudioTimeUs;
    int32_t writeAudioTimeUs;

    // Filters and Gain configuration
    BiquadState stage1;
    BiquadState stage2;
    bool filtersInitialized;
    float outputGain;

    /**
     * @brief Computes coefficients for a 2nd-order Low-Pass Butterworth stage.
     */
    void biquadInitLp(BiquadState &f, float cutoffHz, float q);

    /**
     * @brief Processes a single float sample through a given biquad structural stage.
     */
    float biquadProcess(BiquadState &f, float sampleIn);

    /**
     * @brief Internal static wrapper function required to spin up the FreeRTOS processing task.
     */
    static void audioStreamTaskWrapper(void *pvParameters);

    /**
     * @brief Core background loop running on FreeRTOS to process and flush out audio streams.
     */
    void audioStreamTask();

public:
    /**
     * @brief Constructs a new AudioSystem instance.
     */
    AudioSystem();

    /**
     * @brief Initializes the subsystem architecture and spins up the background worker tasks.
     * @param expander Handle to the IO Expander managing EXIO7 (PA Power).
     * @param codecDev Handle to the pre-initialized ESP Codec Device.
     */
    void init(esp_io_expander_handle_t expander, esp_codec_dev_handle_t codecDev);

    /**
     * @brief Pushes raw lines of audio t-state accumulations into the double buffering loop.
     * @param beeperSamples Pointer to the array containing raw line data (typically 312 samples).
     */
    void pushBeeperSamples(const uint16_t *beeperSamples);

    /**
     * @brief Enables or disables the hardware speaker power amplifier via EXIO7.
     * @param enable True to turn on the amplifier, false to cut power down.
     */
    void setSpeakerPower(bool enable);

    /**
     * @brief Sets the global hardware output volume level.
     * @param volume Target percentage between 0.0f and 100.0f.
     */
    void setSpeakerVolume(float volume);

    // --- Diagnostic Getters for HUD & Logs ---
    inline bool isSpeakerPowered() const { return speakerPowered; }
    inline bool getSpeakerVolume() const { return speakerVolume; }
    inline uint16_t getAudioPeak() const { return audioPeak; }
    inline int32_t getFullAudioTimeUs() const { return fullAudioTimeUs; }
    inline int32_t getWriteAudioTimeUs() const { return writeAudioTimeUs; }
};

#endif // AUDIO_SYSTEM_H

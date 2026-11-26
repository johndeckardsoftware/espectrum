/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#include "audio.h"
#include "config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_check.h"
#include <cmath>

static const char *TAG = "audio";

AudioSystem::AudioSystem()
    : ioExpander(nullptr),
      outputDev(nullptr),
      audioTaskHandle(nullptr),
      audioSamplesIndex(0),
      audioSamplesReady(false),
      speakerPowered(false),
      speakerVolume(50.0f),
      countZeros(0),
      countNzeros(0),
      audioPeak(0),
      fullAudioTimeUs(0),
      writeAudioTimeUs(0),
      filtersInitialized(false),
      outputGain(0.5f)
{
    for (size_t i = 0; i < AUDIO_SAMPLES_SIZE; i++) {
        audioSamples[i] = 0;
    }
}

void AudioSystem::init(esp_io_expander_handle_t expander, esp_codec_dev_handle_t codecDev)
{
    ioExpander = expander;
    outputDev = codecDev;

    // Initialize 4th-order Butterworth digital filter cascade (-24dB/octave response)
    // Cascaded parameters: Stage 1 Q = 0.541196f, Stage 2 Q = 1.306563f
    const float cutoffFreq = 4000.0f; // 4 kHz cutoff frequency
    if (!filtersInitialized) {
        biquadInitLp(stage1, cutoffFreq, 0.541196f);
        biquadInitLp(stage2, cutoffFreq, 1.306563f);
        filtersInitialized = true;
    }

    // Default startup state for hardware
    setSpeakerPower(true);
    int32_t volume = 60;
    Config::get("volume", &volume);
    setSpeakerVolume((float)volume);
    printf("Volume: %ld\n", volume);

    // Create the dedicated, pinned audio execution pipeline thread
    ESP_LOGI(TAG, "Spawning audio streaming background worker thread...");
    xTaskCreate(AudioSystem::audioStreamTaskWrapper, "audio_task", 8192, this, 6, &audioTaskHandle);
    ESP_LOGI(TAG, "Audio subsystem successfully initialized.");
}

void AudioSystem::setSpeakerPower(bool enable)
{
    if (ioExpander == nullptr) return;

    int level = enable ? 1 : 0;
    if (enable) {
        ESP_LOGI(TAG, "Speaker amplifier enabled (EXIO7 HIGH)");
        speakerPowered = true;
    } else {
        ESP_LOGI(TAG, "Speaker amplifier disabled (EXIO7 LOW)");
        speakerPowered = false;
    }

    ESP_ERROR_CHECK(esp_io_expander_set_level(ioExpander, IO_EXPANDER_PIN_NUM_7, level));
    vTaskDelay(pdMS_TO_TICKS(100));
}

void AudioSystem::setSpeakerVolume(float volume)
{
    if (outputDev == nullptr) return;
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(outputDev, volume));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Speaker volume level set to %.0f%%", volume);
    speakerVolume = volume;
}

void AudioSystem::pushBeeperSamples(const uint16_t *beeperSamples)
{
    int i = audioSamplesIndex;
    int j = 0;

    // Copy the raw line data block sequentially inside our buffering limits
    for (; i < audioSamplesIndex + BEEPER_SAMPLES_SIZE; i++, j++) {
        audioSamples[i] = beeperSamples[j];
    }

    audioSamplesIndex = i;

    // If the internal processing chunk block size gets full, swap pointers out
    if (audioSamplesIndex >= AUDIO_SAMPLES_SIZE) {
        audioSamplesIndex = 0;
        audioSamplesReady = true;
    }
}

void AudioSystem::biquadInitLp(BiquadState &f, float cutoffHz, float q)
{
    float w0 = 2.0f * 3.14159265f * cutoffHz / (float)AUDIO_SAMPLE_RATE;
    float alpha = sinf(w0) / (2.0f * q);
    float cos_w0 = cosf(w0);

    float a0 = 1.0f + alpha;
    f.b0 = ((1.0f - cos_w0) * 0.5f) / a0;
    f.b1 = (1.0f - cos_w0) / a0;
    f.b2 = f.b0;
    f.a1 = (-2.0f * cos_w0) / a0;
    f.a2 = (1.0f - alpha) / a0;

    f.x1 = f.x2 = f.y1 = f.y2 = 0.0f;
}

float AudioSystem::biquadProcess(BiquadState &f, float sampleIn)
{
    float sampleOut = f.b0 * sampleIn + f.b1 * f.x1 + f.b2 * f.x2 - f.a1 * f.y1 - f.a2 * f.y2;

    f.x2 = f.x1;
    f.x1 = sampleIn;
    f.y2 = f.y1;
    f.y1 = sampleOut;

    return sampleOut;
}

void AudioSystem::audioStreamTaskWrapper(void *pvParameters)
{
    // Redirect context mapping structure back to class instance execution context
    AudioSystem *instance = static_cast<AudioSystem *>(pvParameters);
    instance->audioStreamTask();
}


void AudioSystem::audioStreamTask()
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(19);

    if (esp_task_wdt_add(NULL) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register audio task into local watchdog system");
    }

    // An I2S frequency of 16,000 Hz requires 320 samples every 20 ms (16,000 / 50 fps)
    const size_t I2S_BUFFER_SIZE = 320 * 2;

    while (1) {
        esp_task_wdt_reset();
        uint64_t startTimeUs = esp_timer_get_time();

        if (audioSamplesReady) {
            if (outputDev != nullptr) {
                uint64_t processTimeUs = esp_timer_get_time();
                int16_t samples[I2S_BUFFER_SIZE];

                // Elementary resampling from 312 to 320 samples
                for (size_t i = 0; i < I2S_BUFFER_SIZE; i++) {
                    // Map the index from 0..319 to 0..311
                    size_t srcIndex = (i * AUDIO_SAMPLES_SIZE) / I2S_BUFFER_SIZE;
                    float inputVal = (float)audioSamples[srcIndex];

                    // Apply the cascade of filters set to 16,000 Hz
                    float out1 = biquadProcess(stage1, inputVal);
                    float filtered = biquadProcess(stage2, out1);

                    samples[i] = (int16_t)(filtered * 64.0f * outputGain);
                }

                // Send the correct 320-sample buffer at 16kHz
                esp_codec_dev_write(outputDev, samples, I2S_BUFFER_SIZE * sizeof(int16_t));
                writeAudioTimeUs = esp_timer_get_time() - processTimeUs;
            }
            audioSamplesReady = false;
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        fullAudioTimeUs = esp_timer_get_time() - startTimeUs;
    }
}

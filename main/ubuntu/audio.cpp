/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */
#include "audio.h"
#include <algorithm>
#include <array>
#include <cmath>

AudioSystem::~AudioSystem()
{
    if (streamReady)
    {
        StopAudioStream(stream);
        UnloadAudioStream(stream);
    }
    if (audioDeviceReady)
        CloseAudioDevice();
}

void AudioSystem::init()
{
    InitAudioDevice();
    audioDeviceReady = IsAudioDeviceReady();
    if (!audioDeviceReady)
    {
        TraceLog(LOG_WARNING, "Audio device unavailable; continuing without audio");
        return;
    }

    SetAudioStreamBufferSizeDefault(BEEPER_SAMPLES_SIZE);
    stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 16, 1);
    streamReady = stream.buffer != nullptr;
    if (streamReady)
        PlayAudioStream(stream);
    else
        TraceLog(LOG_WARNING, "Failed to create the Raylib audio stream");
}

void AudioSystem::pushBeeperSamples(const uint16_t *beeperSamples)
{
    if (!streamReady || !IsAudioStreamProcessed(stream))
        return;

    float mean = 0.0f;
    for (size_t i = 0; i < BEEPER_SAMPLES_SIZE; ++i)
        mean += beeperSamples[i];
    mean /= BEEPER_SAMPLES_SIZE;

    std::array<int16_t, BEEPER_SAMPLES_SIZE> samples{};
    for (size_t i = 0; i < samples.size(); ++i)
    {
        const float centered = static_cast<float>(beeperSamples[i]) - mean;
        const float scaled = centered * 64.0f;
        samples[i] = static_cast<int16_t>(std::clamp(scaled, -16000.0f, 16000.0f));
        audioPeak = std::max<uint16_t>(audioPeak, static_cast<uint16_t>(std::abs(samples[i])));
    }
    UpdateAudioStream(stream, samples.data(), static_cast<int>(samples.size()));
}

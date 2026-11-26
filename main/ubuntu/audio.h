/*
 * Original Author: johndeckardsoftware
 * Original Source: https://github.com/johndeckardsoftware/espectrum
 * Original License: GNU General Public License v3.0
 */

#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#define AUDIO_SAMPLE_RATE    16000
#define BEEPER_SAMPLES_SIZE  312
#define BEEPER_SAMPLES_RATE  (BEEPER_SAMPLES_SIZE * 50)
#define AUDIO_SAMPLES_SIZE   (BEEPER_SAMPLES_SIZE * 2)

#include <raylib.h>

class AudioSystem
{
private:
    AudioStream stream{};
    bool audioDeviceReady = false;
    bool streamReady = false;
    uint16_t audioPeak = 0;

public:
    ~AudioSystem();
    void init();
    void pushBeeperSamples(const uint16_t *beeperSamples);
    inline bool isSpeakerPowered() const { return true; }
    inline bool getSpeakerVolume() const { return true; }
    inline uint16_t getAudioPeak() const { return audioPeak; }
    inline int32_t getFullAudioTimeUs() const { return 0; }
    inline int32_t getWriteAudioTimeUs() const { return 0; }
};

#endif // AUDIO_SYSTEM_H

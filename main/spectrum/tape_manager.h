/*
 * Original Author: Magnus Krook
 * Original Source: https://softspectrum48.weebly.com/
 * Original License: freeware
 *
 * Adapted by:      johndeckardsoftware
 * Adaptation Date: November 2026
 * Modifications:   Adapted for ESP32-S3-Touch-LCD-3.5_Rev2.0 board using ESP32/ESP-IDF platform.
 *
 */

#ifndef TAPE_MANAGER_H
#define TAPE_MANAGER_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include "port.h"
#include "tape.h"
#include "tape_block.h"
#include "tape_signal.h"

class TapeManager {
private:
    Port& port;
    uint8_t *EarInput = &port.micInBit;
    uint8_t *MicFeedBack = &port.micOutBit;
    std::unique_ptr<Tape> _tape;

    uint32_t _pulseDurationCounter = 0;
    uint32_t _pulseCounter = 0;
    TapeSignal _currentSignal;

    void SelectTapeBlock(int blockIndex);

public:
    // Callbacks replacing C# EventHandlers
    std::function<void()> TapeProgress;
    std::function<void()> TapeStatusChange;

    int CurrentTapePosition = 0;
    uint32_t CurrentPulseRepetitions = 0;
    uint32_t CurrentPulseLength = 0;
    bool CurrentSignalIsLeader = false;
    int CurrentBlockIndex = 0;

    bool IsTapePlaying = false;
    bool IsTapePauseHandled = true;
    bool IsRecording = false;
    bool IsOnBlockStart = false;
    bool IsManuallyPaused = false;
    bool IsSpeedLoading = false;
    bool IsOpening = false;

    std::string FilePath;
    std::vector<uint8_t> TapeContent;

    explicit TapeManager(Port& port);

    TapeBlock* GetCurrentBlock();
    TapeBlock* GetNextBlock();

    bool IsTapeEmpty() const { return TapeContent.empty(); }

    const std::vector<TapeBlock>* GetTapeListing() const {
        return _tape ? &_tape->TapeListing : nullptr;
    }

    std::string GetFileName() const;
    std::string GetFileType() const;

    void UpdateInPort(uint8_t* inPort) { EarInput = inPort; }

    void NewTape();
    bool OpenTape(const std::string& file);
    void RefreshTapeContent();
    void RewindTape(int blockIndex);
    void ResetEarInput();
    void PlayTape(bool setAudioToLow);
    void PauseTape();
    void ManuallyPauseTape();

    void ProcessTapeData(uint32_t elapsedTStates);
    void SkipSignal();
    void GoToEndOfBlock(int targetBlock);
    void ToggleInPort();

protected:
    virtual void OnTapeProgress();
    virtual void OnTapeStatusChange();
};

#endif // TAPE_MANAGER_H
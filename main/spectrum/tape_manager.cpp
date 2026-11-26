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

#include "tape_manager.h"
#include "tap_tape.h"
#include "tzx_tape.h"
#include <cstdio>
#include <cctype>

TapeManager::TapeManager(Port& _port)
    : port(_port), IsTapePauseHandled(false)
{
    NewTape();
}

TapeBlock* TapeManager::GetCurrentBlock() {
    if (!_tape || CurrentBlockIndex < 0 || static_cast<size_t>(CurrentBlockIndex) >= _tape->TapeListing.size()) {
        return nullptr;
    }
    return &_tape->TapeListing[CurrentBlockIndex];
}

TapeBlock* TapeManager::GetNextBlock() {
    if (!_tape || _tape->TapeListing.empty()) {
        return nullptr;
    }
    if (CurrentTapePosition <= 0) {
        return &_tape->TapeListing[0];
    } else if (CurrentBlockIndex < static_cast<int>(_tape->TapeListing.size()) - 1) {
        return &_tape->TapeListing[CurrentBlockIndex + 1];
    }
    return nullptr;
}

std::string TapeManager::GetFileName() const {
    if (FilePath.empty()) return "";
    size_t pos = FilePath.find_last_of("/\\");
    return (pos == std::string::npos) ? FilePath : FilePath.substr(pos + 1);
}

std::string TapeManager::GetFileType() const {
    std::string name = GetFileName();
    if (name.length() >= 3) {
        std::string ext = name.substr(name.length() - 3);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
        return ext;
    }
    return "TAP";
}

void TapeManager::NewTape() {
    TapeContent.clear();
    _tape = std::make_unique<TAPTape>(TapeContent);
    FilePath.clear();
    IsRecording = false;
    RewindTape(0);
}

bool TapeManager::OpenTape(const std::string& file) {
    FILE* f = std::fopen(file.c_str(), "rb");
    if (!f) {
        return false;
    }

    PauseTape();
    IsOpening = true;

    FilePath = file;
    std::string fileType = GetFileType();
    for (char& c : fileType) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    std::fseek(f, 0, SEEK_END);
    long fileSize = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    printf("filename: %s, fileSize: %ld\n", file.c_str(), fileSize);

    if (fileSize <= 0) {
        std::fclose(f);
        IsOpening = false;
        return false;
    }

    TapeContent.resize(static_cast<size_t>(fileSize));
    size_t bytesRead = std::fread(TapeContent.data(), 1, TapeContent.size(), f);
    std::fclose(f);

    if (bytesRead != TapeContent.size()) {
        IsOpening = false;
        return false;
    }

    if (fileType == "TAP") {
        _tape = std::make_unique<TAPTape>(TapeContent);
    }
    else if (fileType == "TZX") {
        _tape = std::make_unique<TZXTape>(TapeContent);
    }
    else {
        _tape.reset();
        IsOpening = false;
        return false;
    }

    if (!_tape || _tape->TapeListing.empty()) {
        _tape.reset();
        IsOpening = false;
        return false;
    }

    IsOpening = false;
    IsRecording = false;
    RewindTape(0);

    return true;
}

void TapeManager::RefreshTapeContent() {
    _tape = std::make_unique<TAPTape>(TapeContent);
    IsRecording = true;
    OnTapeStatusChange();
}

void TapeManager::RewindTape(int blockIndex) {
    CurrentBlockIndex = blockIndex;
    if (CurrentBlockIndex == 0 || !_tape || _tape->TapeListing.empty()) {
        CurrentTapePosition = -1;
    } else {
        CurrentTapePosition = _tape->TapeListing[CurrentBlockIndex].StartPosition - 1;
    }

    IsTapePlaying = false;
    IsTapePauseHandled = false;
    IsOnBlockStart = true;
    OnTapeStatusChange();

    _pulseCounter = 0;
    _pulseDurationCounter = 0;
    CurrentPulseLength = 0;
    CurrentPulseRepetitions = 0;
}

void TapeManager::ResetEarInput() {
    if (EarInput) {
        *EarInput = 0x00;
        *MicFeedBack = 0x00;
    }
}

void TapeManager::PlayTape(bool setAudioToLow) {
    if (setAudioToLow && EarInput) {
        *EarInput = 0x00;
        *MicFeedBack = 0x00;
    }

    IsTapePlaying = true;
    IsOnBlockStart = false;
    IsManuallyPaused = false;
    IsRecording = false;
    IsSpeedLoading = false;

    OnTapeStatusChange();
}

void TapeManager::PauseTape() {
    IsTapePlaying = false;
    IsTapePauseHandled = false;
    OnTapeStatusChange();
}

void TapeManager::ManuallyPauseTape() {
    IsTapePlaying = false;
    IsTapePauseHandled = false;
    IsManuallyPaused = true;
    OnTapeStatusChange();
}

void TapeManager::ProcessTapeData(uint32_t elapsedTStates) {
    if (!_tape || !EarInput) return;

    TapeBlock* nextBlock = GetNextBlock();
    if (nextBlock != nullptr && CurrentTapePosition > nextBlock->StartPosition) {
        SelectTapeBlock(CurrentBlockIndex += 1);
    }

    // Carica il primo segnale se il nastro è appena partito
    if (CurrentPulseLength == 0) {
        CurrentTapePosition += 1;
        if (CurrentTapePosition < static_cast<int>(_tape->Signals.size())) {
            _currentSignal = _tape->Signals[CurrentTapePosition];
            CurrentPulseRepetitions = _currentSignal.NumberOfPulses;
            CurrentPulseLength = _currentSignal.PulseLength;
            CurrentSignalIsLeader = _currentSignal.IsLeader;
            _pulseCounter = 0;
            _pulseDurationCounter = 0;
        } else {
            RewindTape(0);
            return;
        }
    }

    _pulseDurationCounter += elapsedTStates;

    while (CurrentPulseLength > 0 && _pulseDurationCounter >= CurrentPulseLength) {
        _pulseDurationCounter -= CurrentPulseLength;
        _pulseCounter += 1;

        if (_currentSignal.Pause) {
            *EarInput = 0x00;
            *MicFeedBack = 0x00;
        } else if (_currentSignal.ForceHigh) {
            *EarInput = 0x40;
            *MicFeedBack = 0x40;
        } else if (_currentSignal.ForceLow) {
            *EarInput = 0x00;
            *MicFeedBack = 0x00;
        } else {
            ToggleInPort();
        }

        if (_currentSignal.StopTape) {
            PauseTape();
            return;
        }

        if (_pulseCounter >= CurrentPulseRepetitions) {
            CurrentTapePosition += 1;

            if (CurrentTapePosition < static_cast<int>(_tape->Signals.size())) {
                _currentSignal = _tape->Signals[CurrentTapePosition];
                CurrentPulseRepetitions = _currentSignal.NumberOfPulses;
                CurrentPulseLength = _currentSignal.PulseLength;
                CurrentSignalIsLeader = _currentSignal.IsLeader;
                _pulseCounter = 0;
            } else {
                RewindTape(0);
                return;
            }
        }
    }
}

void TapeManager::SkipSignal() {
    if (CurrentPulseRepetitions > _pulseCounter + 80) {
        CurrentPulseRepetitions = _pulseCounter + 80;

        if (CurrentPulseRepetitions % 2 != 0) {
            CurrentPulseRepetitions += 1;
        }
    }
}

void TapeManager::GoToEndOfBlock(int targetBlock) {
    if (_tape && targetBlock >= 0 && targetBlock < static_cast<int>(_tape->TapeListing.size()) - 1) {
        SelectTapeBlock(targetBlock);
        TapeBlock* curr = GetCurrentBlock();
        if (curr) {
            CurrentTapePosition = curr->StartPosition + curr->NumberOfPulses - 1;
        }
    } else {
        RewindTape(0);
    }
}

void TapeManager::ToggleInPort() {
    if (!EarInput) return;
    *EarInput = (*EarInput == 0x40) ? 0x00 : 0x40;
    *MicFeedBack = (*MicFeedBack == 0x40) ? 0x00 : 0x40;
}

void TapeManager::OnTapeProgress() {
    if (TapeProgress) {
        TapeProgress();
    }
}

void TapeManager::OnTapeStatusChange() {
    if (TapeStatusChange) {
        TapeStatusChange();
    }
}

void TapeManager::SelectTapeBlock(int blockIndex) {
    CurrentBlockIndex = blockIndex;
    OnTapeProgress();
}

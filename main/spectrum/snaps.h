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

 // snaps.h
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "zx48k.h"

class Snaps {
public:
    explicit Snaps(ZXSpectrum* spectrum);

    void loadData(const unsigned char *data, size_t size);

    bool loadSnapshot(const std::string& filename, std::string* errorMessage = nullptr);
    bool saveSnapshot(const std::string& filename, std::string* errorMessage = nullptr);

    bool parseZ80File(std::string* errorMessage = nullptr);
    bool parseSnaFile(std::string* errorMessage = nullptr);
    bool parseSzxFile(std::string* errorMessage = nullptr);

    std::vector<uint8_t> createSnapshot();

    int model() const { return model_; }
    uint32_t tstates() const { return tstates_; }
    uint8_t pagingFlags() const { return pagingFlags_; }

private:
    ZXSpectrum* spectrum_;
    std::vector<uint8_t> data_;
    int model_ = 0;
    uint32_t tstates_ = 0;
    uint32_t frameTStates_ = 0;
    bool halted_ = false;
    uint8_t pagingFlags_ = 0;

    static uint8_t u8(const std::vector<uint8_t>& buf, size_t offset);
    static uint16_t u16le(const std::vector<uint8_t>& buf, size_t offset);
    static uint16_t u16be(const std::vector<uint8_t>& buf, size_t offset);
    static uint32_t u32le(const std::vector<uint8_t>& buf, size_t offset);
    static uint32_t u32be(const std::vector<uint8_t>& buf, size_t offset);

    std::vector<uint8_t> extractMemoryBlock(size_t fileOffset, bool isCompressed, size_t unpackedLength) const;
    static std::vector<uint8_t> decompressZlib(const uint8_t* src, size_t srcLen, size_t expectedOutLen);
    static void compressZ80Bank(const uint8_t* bankData, size_t pageSize, std::vector<uint8_t>& out);
    static std::string readFourCC(const std::vector<uint8_t>& buf, size_t offset);
};
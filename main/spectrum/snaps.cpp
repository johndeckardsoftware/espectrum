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

// snaps.cpp
#include "snaps.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <zlib.h>

namespace {
constexpr size_t kPageSize = 0x4000; // 16384 bytes
}

Snaps::Snaps(ZXSpectrum* spectrum) : spectrum_(spectrum) {}

uint8_t Snaps::u8(const std::vector<uint8_t>& buf, size_t offset) {
    return buf.at(offset);
}

uint16_t Snaps::u16le(const std::vector<uint8_t>& buf, size_t offset) {
    return static_cast<uint16_t>(buf.at(offset)) |
           (static_cast<uint16_t>(buf.at(offset + 1)) << 8);
}

uint16_t Snaps::u16be(const std::vector<uint8_t>& buf, size_t offset) {
    return (static_cast<uint16_t>(buf.at(offset)) << 8) |
           static_cast<uint16_t>(buf.at(offset + 1));
}

uint32_t Snaps::u32le(const std::vector<uint8_t>& buf, size_t offset) {
    return static_cast<uint32_t>(buf.at(offset)) |
           (static_cast<uint32_t>(buf.at(offset + 1)) << 8) |
           (static_cast<uint32_t>(buf.at(offset + 2)) << 16) |
           (static_cast<uint32_t>(buf.at(offset + 3)) << 24);
}

uint32_t Snaps::u32be(const std::vector<uint8_t>& buf, size_t offset) {
    return (static_cast<uint32_t>(buf.at(offset)) << 24) |
           (static_cast<uint32_t>(buf.at(offset + 1)) << 16) |
           (static_cast<uint32_t>(buf.at(offset + 2)) << 8) |
           static_cast<uint32_t>(buf.at(offset + 3));
}

std::string Snaps::readFourCC(const std::vector<uint8_t>& buf, size_t offset) {
    return {
        static_cast<char>(u8(buf, offset)),
        static_cast<char>(u8(buf, offset + 1)),
        static_cast<char>(u8(buf, offset + 2)),
        static_cast<char>(u8(buf, offset + 3))
    };
}

std::vector<uint8_t> Snaps::extractMemoryBlock(size_t fileOffset, bool isCompressed, size_t unpackedLength) const {
    if (!isCompressed) {
        if (fileOffset + unpackedLength > data_.size()) {
            return {};
        }
        return std::vector<uint8_t>(data_.begin() + static_cast<ptrdiff_t>(fileOffset),
                                    data_.begin() + static_cast<ptrdiff_t>(fileOffset + unpackedLength));
    }

    std::vector<uint8_t> memoryBytes(unpackedLength, 0);
    size_t filePtr = fileOffset;
    size_t memPtr = 0;

    while (memPtr < unpackedLength && filePtr < data_.size()) {
        if ((unpackedLength - memPtr >= 2) &&
            (filePtr + 3 < data_.size()) &&
            (data_[filePtr] == 0xED) &&
            (data_[filePtr + 1] == 0xED)) {
            const uint8_t count = data_[filePtr + 2];
            const uint8_t value = data_[filePtr + 3];
            for (uint8_t i = 0; i < count && memPtr < unpackedLength; ++i) {
                memoryBytes[memPtr++] = value;
            }
            filePtr += 4;
        } else {
            memoryBytes[memPtr++] = data_[filePtr++];
        }
    }

    return memoryBytes;
}

std::vector<uint8_t> Snaps::decompressZlib(const uint8_t* src, size_t srcLen, size_t expectedOutLen) {
    std::vector<uint8_t> out(expectedOutLen, 0);
    uLongf destLen = static_cast<uLongf>(expectedOutLen);
    const int res = ::uncompress(out.data(), &destLen, src, static_cast<uLong>(srcLen));
    if (res != Z_OK) {
        return {};
    }
    out.resize(destLen);
    return out;
}

bool Snaps::parseZ80File(std::string* errorMessage) {
    if (data_.size() < 30) {
        if (errorMessage) *errorMessage = "parseZ80File: file too small for Z80 header";
        return false;
    }

    auto& reg = spectrum_->z80.reg;

    reg.pair.A = u8(data_, 0);
    reg.pair.F = u8(data_, 1);
    reg.pair.C = u8(data_, 2);
    reg.pair.B = u8(data_, 3);
    reg.pair.L = u8(data_, 4);
    reg.pair.H = u8(data_, 5);
    reg.PC = u16le(data_, 6); // Fixed: offset 6-7, not 5
    reg.SP = u16le(data_, 8);

    uint8_t I = u8(data_, 10);
    uint8_t R = u8(data_, 11) & 0x7F;

    uint8_t flags1 = u8(data_, 12);
    if (flags1 == 255)
        flags1 = 1;
    if (flags1 & 0x01)
        R |= 0x80;

    reg.I = I;
    reg.R = R;
    reg.pair.E = u8(data_, 13);
    reg.pair.D = u8(data_, 14);
    reg.back.C = u8(data_, 15);
    reg.back.B = u8(data_, 16);
    reg.back.E = u8(data_, 17);
    reg.back.D = u8(data_, 18);
    reg.back.L = u8(data_, 19);
    reg.back.H = u8(data_, 20);
    reg.back.A = u8(data_, 21);
    reg.back.F = u8(data_, 22);

    reg.IY = u16le(data_, 23);
    reg.IX = u16le(data_, 25);

    uint8_t IFF1 = u8(data_, 27);
    uint8_t IFF2 = u8(data_, 28);
    uint8_t IM = u8(data_, 29) & 0x03;

    reg.IFF = (IFF1 ? 0x01 : 0x00) | (IFF2 ? 0x04 : 0x00);
    reg.interrupt = (reg.interrupt & 0b11111100) | IM;
    reg.execEI = 0;

    if (reg.PC != 0) {
        // Version 1 snapshot
        model_ = 48;
        const bool isCompressed = (u8(data_, 12) & 0x20) != 0;
        const std::vector<uint8_t> memory = extractMemoryBlock(30, isCompressed, 0xC000);
        if (memory.size() < 0xC000) {
            if (errorMessage) *errorMessage = "parseZ80File: truncated v1 memory block";
            return false;
        }

        spectrum_->mem.loadRamPage(1, memory.data() + 0x0000);
        spectrum_->mem.loadRamPage(2, memory.data() + 0x4000);
        spectrum_->mem.loadRamPage(3, memory.data() + 0x8000);
        tstates_ = 0;
    } else {
        // Version 2 / 3 snapshot
        if (data_.size() < 34) {
            if (errorMessage) *errorMessage = "parseZ80File: truncated v2/v3 header";
            return false;
        }
        const uint16_t additionalHeaderLength = u16le(data_, 30);
        const bool isVersion2 = (additionalHeaderLength == 23);
        reg.PC = u16le(data_, 32);
        const uint8_t machineId = u8(data_, 34);
        const bool is48K = isVersion2 ? (machineId < 3) : (machineId < 4);
        model_ = is48K ? 48 : 128;

        if (!is48K && data_.size() > 35) {
            pagingFlags_ = u8(data_, 35);
        }

        if (data_.size() > 57) {
            const uint32_t tstateChunkSize = (is48K ? 69888U : 70908U) / 4U;
            const uint32_t counter = u16le(data_, 55);
            const uint32_t quarter = ((static_cast<uint32_t>(u8(data_, 57)) + 1U) % 4U) + 1U;
            const uint32_t rawTstates = quarter * tstateChunkSize - (counter + 1U);
            tstates_ = (rawTstates >= tstateChunkSize * 4U) ? 0U : rawTstates;
        } else {
            tstates_ = 0;
        }

        size_t offset = 32 + static_cast<size_t>(additionalHeaderLength);
        while (offset + 3 <= data_.size()) {
            uint16_t compressedLength = u16le(data_, offset);
            bool isCompressed = true;
            if (compressedLength == 0xFFFF) {
                compressedLength = static_cast<uint16_t>(kPageSize);
                isCompressed = false;
            }

            const uint8_t pageNumber = u8(data_, offset + 2);
            if (offset + 3 + compressedLength > data_.size()) {
                break;
            }

            // page renumbering based on actual memory configuration
            int slot = -1;
            if (model_ == 48) {
                if (pageNumber == 4) slot = 2;
                else if (pageNumber == 5) slot = 3;
                else if (pageNumber == 8) slot = 1;
            } else {
                if (pageNumber >= 3 && pageNumber <= 10) {
                    slot = slot - 3;
                }
            }

            if (slot >= 0) {
                std::vector<uint8_t> pageData = extractMemoryBlock(offset + 3, isCompressed, kPageSize);
                if (pageData.size() == kPageSize) {
                    spectrum_->mem.loadRamPage(slot, pageData.data());
                }
            }

            offset += 3 + static_cast<size_t>(compressedLength);
        }
    }

    if (model_ != 48) {
        spectrum_->port.out(0x7FFD, pagingFlags_);
    }

    spectrum_->port.borderColor = (flags1 >> 1) & 0x07;

    return true;
}

bool Snaps::parseSnaFile(std::string* errorMessage) {
    constexpr size_t kHeaderSize = 27;
    constexpr size_t kSna48Size = 49179;
    constexpr size_t kSna128A = 131103;
    constexpr size_t kSna128B = 147487;

    bool mode128 = false;
    if (data_.size() == kSna128A || data_.size() == kSna128B) {
        mode128 = true;
    } else if (data_.size() == kSna48Size) {
        mode128 = false;
    } else {
        if (errorMessage) {
            *errorMessage = "parseSnaFile: unsupported SNA length " + std::to_string(data_.size());
        }
        return false;
    }

    model_ = mode128 ? 128 : 48;
    tstates_ = 0;

    std::vector<int> loadedPages;
    loadedPages.reserve(8);

    if (mode128) {
        spectrum_->mem.loadRamPage(5, data_.data() + kHeaderSize);
        spectrum_->mem.loadRamPage(2, data_.data() + kHeaderSize + kPageSize);
        loadedPages.push_back(2);
        loadedPages.push_back(5);

        const int page = u8(data_, 49181) & 0x07;
        spectrum_->mem.loadRamPage(page, data_.data() + kHeaderSize + (2 * kPageSize));
        if (std::find(loadedPages.begin(), loadedPages.end(), page) == loadedPages.end()) {
            loadedPages.push_back(page);
        }

        size_t ptr = 49183;
        for (int bank = 0; bank < 8; ++bank) {
            if (std::find(loadedPages.begin(), loadedPages.end(), bank) == loadedPages.end()) {
                if (ptr + kPageSize <= data_.size()) {
                    spectrum_->mem.loadRamPage(bank, data_.data() + ptr);
                }
                loadedPages.push_back(bank);
                ptr += kPageSize;
            }
        }
    } else {
        // page/slot index rearranged based on actual memory configuration
        spectrum_->mem.loadRamPage(1, data_.data() + kHeaderSize);
        spectrum_->mem.loadRamPage(2, data_.data() + kHeaderSize + kPageSize);
        spectrum_->mem.loadRamPage(3, data_.data() + kHeaderSize + (2 * kPageSize));
    }

    auto& reg = spectrum_->z80.reg;
    reg.I = u8(data_, 0);
    reg.back.L = u8(data_, 1);
    reg.back.H = u8(data_, 2);
    reg.back.E = u8(data_, 3);
    reg.back.D = u8(data_, 4);
    reg.back.C = u8(data_, 5);
    reg.back.B = u8(data_, 6);
    reg.back.F = u8(data_, 7);
    reg.back.A = u8(data_, 8);

    reg.pair.L = u8(data_, 9);
    reg.pair.H = u8(data_, 10);
    reg.pair.E = u8(data_, 11);
    reg.pair.D = u8(data_, 12);
    reg.pair.C = u8(data_, 13);
    reg.pair.B = u8(data_, 14);

    reg.IY = u16le(data_, 15);
    reg.IX = u16le(data_, 17);

    if (u8(data_, 19) & 0x04) {
        reg.IFF |= 0x05; // IFF1 | IFF2
    } else {
        reg.IFF &= static_cast<uint8_t>(~0x05);
    }

    reg.R = u8(data_, 20);
    reg.pair.F = u8(data_, 21);
    reg.pair.A = u8(data_, 22);

    const uint8_t im = u8(data_, 25) & 0x03;
    reg.interrupt = static_cast<uint8_t>((reg.interrupt & 0xFC) | im);
    spectrum_->port.borderColor = u8(data_, 26);

    if (mode128) {
        reg.SP = u16le(data_, 23);
        reg.PC = u16le(data_, 49179);
        pagingFlags_ = u8(data_, 49181);
        spectrum_->port.out(0x7FFD, pagingFlags_);
    } else {
        uint16_t sp = u16le(data_, 23);
        const size_t loIdx = static_cast<size_t>((static_cast<int>(sp) - 16384) + static_cast<int>(kHeaderSize));
        const uint8_t lo = u8(data_, loIdx);
        sp = static_cast<uint16_t>((sp + 1U) & 0xFFFFU);
        const size_t hiIdx = static_cast<size_t>((static_cast<int>(sp) - 16384) + static_cast<int>(kHeaderSize));
        const uint8_t hi = u8(data_, hiIdx);
        sp = static_cast<uint16_t>((sp + 1U) & 0xFFFFU);
        reg.PC = static_cast<uint16_t>((static_cast<uint16_t>(hi) << 8) | lo);
        reg.SP = sp;
    }

    return true;
}

bool Snaps::parseSzxFile(std::string* errorMessage) {
    if (data_.size() < 8 || readFourCC(data_, 0) != "ZXST") {
        if (errorMessage) *errorMessage = "parseSzxFile: not a valid SZX file";
        return false;
    }

    const uint8_t machineId = u8(data_, 6);
    switch (machineId) {
        case 1: model_ = 48; break;
        case 2:
        case 3: model_ = 128; break;
        case 7: model_ = 5; break;
        default:
            if (errorMessage) {
                *errorMessage = "parseSzxFile: unsupported machine type " + std::to_string(machineId);
            }
            return false;
    }

    auto& reg = spectrum_->z80.reg;
    size_t offset = 8;

    while (offset + 8 <= data_.size()) {
        const std::string blockId = readFourCC(data_, offset);
        const uint32_t blockLen = u32le(data_, offset + 4);
        offset += 8;

        if (offset + blockLen > data_.size()) {
            break;
        }

        if (blockId == "Z80R" && blockLen >= 37) {
            reg.pair.F = u8(data_, offset + 0);
            reg.pair.A = u8(data_, offset + 1);
            reg.pair.C = u8(data_, offset + 2);
            reg.pair.B = u8(data_, offset + 3);
            reg.pair.E = u8(data_, offset + 4);
            reg.pair.D = u8(data_, offset + 5);
            reg.pair.L = u8(data_, offset + 6);
            reg.pair.H = u8(data_, offset + 7);

            reg.back.F = u8(data_, offset + 8);
            reg.back.A = u8(data_, offset + 9);
            reg.back.C = u8(data_, offset + 10);
            reg.back.B = u8(data_, offset + 11);
            reg.back.E = u8(data_, offset + 12);
            reg.back.D = u8(data_, offset + 13);
            reg.back.L = u8(data_, offset + 14);
            reg.back.H = u8(data_, offset + 15);

            reg.IX = u16le(data_, offset + 16);
            reg.IY = u16le(data_, offset + 18);
            reg.SP = u16le(data_, offset + 20);
            reg.PC = u16le(data_, offset + 22);
            reg.R = u8(data_, offset + 24);
            reg.I = u8(data_, offset + 25);

            if (u8(data_, offset + 26) & 0x01) reg.IFF |= 0x01;
            else reg.IFF &= static_cast<uint8_t>(~0x01);

            if (u8(data_, offset + 27) & 0x01) reg.IFF |= 0x04;
            else reg.IFF &= static_cast<uint8_t>(~0x04);

            reg.interrupt = static_cast<uint8_t>((reg.interrupt & 0xFC) | (u8(data_, offset + 28) & 0x03));
            frameTStates_ = u32le(data_, offset + 29);
            tstates_ = frameTStates_;
            //reg.consumeClockCounter = tstates_;
            reg.WZ = u16le(data_, offset + 35);
        } else if (blockId == "SPCR" && blockLen >= 2) {
            spectrum_->port.borderColor = u8(data_, offset + 0);
            pagingFlags_ = u8(data_, offset + 1);
        } else if (blockId == "RAMP" && blockLen >= 3) {
            const uint16_t flags = u16le(data_, offset + 0);
            const bool isCompressed = (flags & 0x0001) != 0;
            int pageNumber = static_cast<int>(u8(data_, offset + 2));

            // page renumbering based on actual memory configuration
            int slot = -1;
            if (model_ == 48) {
                if (pageNumber == 0) slot = 3;
                else if (pageNumber == 2) slot = 2;
                else if (pageNumber == 5) slot = 1;
            } else {
                if (pageNumber >= 0 && pageNumber <= 7) {
                    slot = pageNumber;
                }
            }

            if (slot >= 0) {
                if (isCompressed && blockLen > 3) {
                    const size_t compLen = static_cast<size_t>(blockLen - 3);
                    std::vector<uint8_t> pageData = decompressZlib(data_.data() + offset + 3, compLen, kPageSize);
                    if (pageData.size() == kPageSize) {
                        spectrum_->mem.loadRamPage(slot, pageData.data());
                    }
                } else if (!isCompressed && blockLen >= 3 + kPageSize) {
                    spectrum_->mem.loadRamPage(slot, data_.data() + offset + 3);
                }
            }
        }

        offset += static_cast<size_t>(blockLen);
    }

    if (model_ != 48) {
        spectrum_->port.out(0x7FFD, pagingFlags_);
    }

    return true;
}

void Snaps::compressZ80Bank(const uint8_t* bankData, size_t pageSize, std::vector<uint8_t>& out) {
    out.clear();
    out.reserve(pageSize);

    size_t bankPos = 0;
    bool previousByteSingleEd = false;

    while (bankPos < pageSize) {
        const uint8_t content = bankData[bankPos];

        if (previousByteSingleEd && content != 0xED) {
            out.push_back(content);
            previousByteSingleEd = false;
            ++bankPos;
        } else {
            size_t repeats = 1;
            while (bankPos + repeats < pageSize &&
                   repeats < 0xFF &&
                   bankData[bankPos + repeats] == content) {
                ++repeats;
            }

            if (repeats >= 5 || (content == 0xED && repeats >= 2)) {
                out.push_back(0xED);
                out.push_back(0xED);
                out.push_back(static_cast<uint8_t>(repeats));
                out.push_back(content);
            } else {
                out.insert(out.end(), repeats, content);
            }

            previousByteSingleEd = (repeats == 1 && content == 0xED);
            bankPos += repeats;
        }
    }
}

std::vector<uint8_t> Snaps::createSnapshot() {
    const auto& reg = spectrum_->z80.reg;
    const size_t pageSize = spectrum_->mem.getPageSize();
    const bool is48K = (spectrum_->type == 48);

    std::vector<uint8_t> snapshot(86 + (is48K ? 3 : 8) * (pageSize + 16), 0);

    snapshot[0] = reg.pair.A;
    snapshot[1] = reg.pair.F;
    snapshot[2] = reg.pair.C;
    snapshot[3] = reg.pair.B;
    snapshot[4] = reg.pair.L;
    snapshot[5] = reg.pair.H;
    snapshot[6] = 0; // Version 2/3 marker
    snapshot[7] = 0;
    snapshot[8] = static_cast<uint8_t>(reg.SP & 0xFF);
    snapshot[9] = static_cast<uint8_t>(reg.SP >> 8);
    snapshot[10] = reg.I;
    snapshot[11] = reg.R;
    snapshot[12] = static_cast<uint8_t>(((reg.R & 0x80) >> 7) | ((spectrum_->port.borderColor & 0x07) << 1) | 0x20);
    snapshot[13] = reg.pair.E;
    snapshot[14] = reg.pair.D;
    snapshot[15] = reg.back.C;
    snapshot[16] = reg.back.B;
    snapshot[17] = reg.back.E;
    snapshot[18] = reg.back.D;
    snapshot[19] = reg.back.L;
    snapshot[20] = reg.back.H;
    snapshot[21] = reg.back.A;
    snapshot[22] = reg.back.F;
    snapshot[23] = static_cast<uint8_t>(reg.IY & 0xFF);
    snapshot[24] = static_cast<uint8_t>(reg.IY >> 8);
    snapshot[25] = static_cast<uint8_t>(reg.IX & 0xFF);
    snapshot[26] = static_cast<uint8_t>(reg.IX >> 8);

    snapshot[27] = (reg.IFF & 0x01) ? 1 : 0;
    snapshot[28] = (reg.IFF & 0x04) ? 1 : 0;
    snapshot[29] = static_cast<uint8_t>(reg.interrupt & 0x03);

    snapshot[30] = 54; // Additional header length (v3)
    snapshot[31] = 0;
    snapshot[32] = static_cast<uint8_t>(reg.PC & 0xFF);
    snapshot[33] = static_cast<uint8_t>(reg.PC >> 8);

    if (is48K) {
        snapshot[34] = 0;
        snapshot[35] = 0;
    } else {
        snapshot[34] = 4; // Spectrum 128
        snapshot[35] = spectrum_->port.lastPagingValue;
        snapshot[38] = spectrum_->port.lastAyRegister;
        for (uint8_t r = 0; r < 16; ++r) {
            spectrum_->ay.selectRegister(r);
            snapshot[39 + r] = spectrum_->ay.getRegisterData();
        }
    }

    std::array<int, 8> bankToPageMap = {99, 99, 99, 99, 99, 99, 99, 99};
    if (is48K) {
        bankToPageMap[1] = 8;
        bankToPageMap[2] = 4;
        bankToPageMap[3] = 5;
    } else {
        for (int b = 0; b < 8; ++b) {
            bankToPageMap[b] = 3 + b;
        }
    }

    size_t storePos = 30 + static_cast<size_t>(snapshot[30]) + 2;
    std::vector<uint8_t> compressedPage;

    for (int bank = 0; bank < 8; ++bank) {
        if (bankToPageMap[bank] == 99) {
            continue;
        }
        const uint8_t* bankData = spectrum_->mem.getBankRam(bank);
        if (!bankData) {
            continue;
        }

        compressZ80Bank(bankData, pageSize, compressedPage);
        const uint16_t packedLen = static_cast<uint16_t>(compressedPage.size());

        if (storePos + 3 + compressedPage.size() > snapshot.size()) {
            snapshot.resize(storePos + 3 + compressedPage.size());
        }

        snapshot[storePos + 0] = static_cast<uint8_t>(packedLen & 0xFF);
        snapshot[storePos + 1] = static_cast<uint8_t>(packedLen >> 8);
        snapshot[storePos + 2] = static_cast<uint8_t>(bankToPageMap[bank]);
        std::memcpy(snapshot.data() + storePos + 3, compressedPage.data(), compressedPage.size());
        storePos += 3 + compressedPage.size();
    }

    snapshot.resize(storePos);
    return snapshot;
}

void Snaps::loadData(const unsigned char * data, size_t size) {
    if (!data || size == 0) {
        data_.clear();
        return;
    }
    data_.assign(data, data + size);
}

bool Snaps::loadSnapshot(const std::string& filename, std::string* errorMessage) {
    std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
    if (!ifs) {
        if (errorMessage) *errorMessage = "loadSnapshot: cannot open file " + filename;
        return false;
    }

    const std::streamsize fileSize = ifs.tellg();
    if (fileSize <= 0) {
        if (errorMessage) *errorMessage = "loadSnapshot: empty or invalid file";
        return false;
    }

    data_.resize(static_cast<size_t>(fileSize));
    ifs.seekg(0, std::ios::beg);
    if (!ifs.read(reinterpret_cast<char*>(data_.data()), fileSize)) {
        if (errorMessage) *errorMessage = "loadSnapshot: failed reading file";
        return false;
    }

    std::string ext;
    const size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        ext = filename.substr(dotPos);
        for (char& c : ext) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    bool ok = false;
    if (ext == ".z80") {
        ok = parseZ80File(errorMessage);
    } else if (ext == ".szx") {
        ok = parseSzxFile(errorMessage);
    } else if (ext == ".sna") {
        ok = parseSnaFile(errorMessage);
    } else {
        if (errorMessage) *errorMessage = "loadSnapshot: unknown snapshot format";
        return false;
    }

    if (!ok) {
        return false;
    }

    if (model_ != spectrum_->type) {
        if (errorMessage) {
            *errorMessage = "Snapshot only for ZX Spectrum " + std::to_string(model_);
        }
        return false;
    }

    return true;
}

bool Snaps::saveSnapshot(const std::string& filename, std::string* errorMessage) {
    const std::vector<uint8_t> snap = createSnapshot();
    if (snap.empty()) {
        if (errorMessage) *errorMessage = "saveSnapshot: failed to build snapshot";
        return false;
    }

    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        if (errorMessage) *errorMessage = "saveSnapshot: cannot open output file " + filename;
        return false;
    }

    ofs.write(reinterpret_cast<const char*>(snap.data()), static_cast<std::streamsize>(snap.size()));
    if (!ofs) {
        if (errorMessage) *errorMessage = "saveSnapshot: write failed";
        return false;
    }

    return true;
}
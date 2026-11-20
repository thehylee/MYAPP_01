
#pragma once

#include "DirectoryScanner.h"

#include <cstdint>
#include <string>
#include <vector>

namespace FAT32Recovery
{
    struct LFNEntry
    {
        uint8_t sequence = 0;

        uint8_t checksum = 0;

        bool isLast = false;

        bool deleted = false;

        std::vector<uint16_t> characters;

        uint64_t entryOffset = 0;
    };

    struct LFNResult
    {
        std::string longName;

        bool valid = false;

        bool checksumValid = false;

        bool sequenceValid = false;

        bool deleted = false;

        uint64_t firstEntryOffset = 0;

        uint64_t lastEntryOffset = 0;
    };

    class LFNParser
    {
    public:

        // LFN 엔트리 목록으로 파일명 복원
        LFNResult Parse(
            const std::vector<DirectoryEntry>& entries,
            size_t sfnIndex
        ) const;

        // SFN 체크섬 계산
        static uint8_t CalculateChecksum(
            const uint8_t* shortName
        );

        // UTF-16 -> UTF-8
        static std::string UTF16ToUTF8(
            const std::vector<uint16_t>& characters
        );

    private:

        bool ExtractCharacters(
            const DirectoryEntry& entry,
            std::vector<uint16_t>& output
        ) const;
    };
}


#pragma once

#include "BPBParser.h"

#include <cstdint>
#include <vector>

namespace FAT32Recovery
{
    enum class FATEntryType
    {
        Free,
        Reserved,
        NextCluster,
        BadCluster,
        EndOfChain,
        Invalid
    };

    struct FATEntry
    {
        uint32_t cluster = 0;

        uint32_t rawValue = 0;

        uint32_t value = 0;

        uint64_t imageOffset = 0;

        FATEntryType type = FATEntryType::Invalid;

        bool valid = false;
    };

    struct FATComparison
    {
        uint32_t cluster = 0;

        FATEntry fat1;

        FATEntry fat2;

        bool match = false;
    };

    class FATReader
    {
    public:

        FATReader(
            DiskImage& image,
            const FAT32Info& info
        );

        // FAT1 엔트리 읽기
        bool ReadFAT1(
            uint32_t cluster,
            FATEntry& entry
        );

        // FAT2 엔트리 읽기
        bool ReadFAT2(
            uint32_t cluster,
            FATEntry& entry
        );

        // 두 FAT 비교
        bool CompareEntry(
            uint32_t cluster,
            FATComparison& result
        );

        // 엔트리 상태 판별
        FATEntryType GetEntryType(
            uint32_t value
        ) const;

        // 상태 이름
        static const char* GetTypeName(
            FATEntryType type
        );

        uint32_t GetMaximumCluster() const;

    private:

        DiskImage& m_image;

        const FAT32Info& m_info;

        bool ReadEntry(
            uint32_t cluster,
            uint64_t fatStart,
            FATEntry& entry
        );
    };
}

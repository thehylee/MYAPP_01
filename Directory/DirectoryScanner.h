
#pragma once

#include "../Core/DiskImage.h"

#include <cstdint>
#include <string>
#include <vector>

namespace FAT32Recovery
{
    enum class DirectoryEntryType
    {
        SFN,
        LFN,
        Deleted,
        VolumeLabel,
        EndOfDirectory,
        Invalid
    };

    struct DirectoryEntry
    {
        uint64_t entryOffset = 0;

        DirectoryEntryType type =
            DirectoryEntryType::Invalid;

        std::string shortName;

        uint8_t attributes = 0;

        uint16_t firstClusterHigh = 0;
        uint16_t firstClusterLow = 0;

        uint32_t startCluster = 0;

        uint32_t fileSize = 0;

        bool deleted = false;

        bool isDirectory = false;

        bool isVolumeLabel = false;

        std::vector<Byte> rawData;
    };

    class DirectoryScanner
    {
    public:

        explicit DirectoryScanner(
            DiskImage& image
        );

        // 특정 디렉터리 데이터 검색
        bool ScanDirectory(
            uint64_t offset,
            uint64_t size
        );

        // 수집된 엔트리 반환
        const std::vector<DirectoryEntry>&
        GetEntries() const;

        // 결과 출력
        void PrintEntries() const;

    private:

        DiskImage& m_image;

        std::vector<DirectoryEntry> m_entries;

        bool ParseEntry(
            const std::vector<Byte>& data,
            uint64_t offset,
            DirectoryEntry& entry
        );
    };
}

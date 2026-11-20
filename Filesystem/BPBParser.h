
#pragma once

#include "../Core/DiskImage.h"

#include <cstdint>
#include <string>

namespace FAT32Recovery
{

    struct PartitionInfo
    {
        uint8_t bootIndicator = 0;
        uint8_t partitionType = 0;

        uint32_t startLBA = 0;
        uint32_t totalSectors = 0;

        bool valid = false;
    };

    struct FAT32Info
    {
        uint32_t partitionStart = 0;
        uint32_t partitionSectors = 0;

        uint16_t bytesPerSector = 0;
        uint8_t sectorsPerCluster = 0;
        uint16_t reservedSectors = 0;
        uint8_t numberOfFATs = 0;

        uint32_t totalSectors = 0;
        uint32_t fatSize = 0;

        uint16_t extFlags = 0;
        uint16_t fsVersion = 0;

        uint32_t rootCluster = 0;

        uint16_t fsInfoSector = 0;
        uint16_t backupBootSector = 0;

        uint64_t fat1Start = 0;
        uint64_t fat2Start = 0;

        uint64_t firstDataSector = 0;

        uint64_t dataSectors = 0;

        uint64_t clusterCount = 0;
        uint32_t maximumCluster = 0;

        uint64_t clusterSize = 0;

        bool valid = false;
    };

    class BPBParser
    {
    public:

        explicit BPBParser(DiskImage& image);

        bool Analyze();

        const PartitionInfo& GetPartition() const;

        const FAT32Info& GetInfo() const;

        void PrintReport() const;

    private:

        DiskImage& m_image;

        PartitionInfo m_partition;

        FAT32Info m_info;

        bool ParseMBR();

        bool ParseBPB();

        bool ValidateBPB();

        static uint16_t ReadLE16(
            const uint8_t* data
        );

        static uint32_t ReadLE32(
            const uint8_t* data
        );
    };

}

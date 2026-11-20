
#include "BPBParser.h"

#include <iostream>
#include <iomanip>
#include <limits>

namespace FAT32Recovery
{

    BPBParser::BPBParser(DiskImage& image)
        : m_image(image)
    {
    }

    uint16_t BPBParser::ReadLE16(const uint8_t* data)
    {
        return static_cast<uint16_t>(
            data[0] |
            (static_cast<uint16_t>(data[1]) << 8)
        );
    }

    uint32_t BPBParser::ReadLE32(const uint8_t* data)
    {
        return
            static_cast<uint32_t>(data[0]) |
            (static_cast<uint32_t>(data[1]) << 8) |
            (static_cast<uint32_t>(data[2]) << 16) |
            (static_cast<uint32_t>(data[3]) << 24);
    }

    bool BPBParser::Analyze()
    {
        m_partition = {};
        m_info = {};

        if (!m_image.IsOpen())
        {
            std::cerr << "ERROR: Image is not open.\n";
            return false;
        }

        if (!ParseMBR())
        {
            return false;
        }

        if (!ParseBPB())
        {
            return false;
        }

        if (!ValidateBPB())
        {
            return false;
        }

        m_info.valid = true;

        return true;
    }

    bool BPBParser::ParseMBR()
    {
        std::vector<Byte> sector;

        if (m_image.ReadSector(0, 512, sector)
            != ReadStatus::Success)
        {
            std::cerr << "ERROR: Cannot read MBR.\n";
            return false;
        }

        if (sector.size() != 512)
            return false;

        // MBR signature
        if (sector[510] != 0x55 ||
            sector[511] != 0xAA)
        {
            std::cerr << "ERROR: Invalid MBR signature.\n";
            return false;
        }

        // Primary partition entries
        for (int i = 0; i < 4; ++i)
        {
            const size_t offset = 446 + i * 16;

            const uint8_t type = sector[offset + 4];

            if (type != 0x0B && type != 0x0C)
                continue;

            m_partition.bootIndicator =
                sector[offset];

            m_partition.partitionType = type;

            m_partition.startLBA =
                ReadLE32(&sector[offset + 8]);

            m_partition.totalSectors =
                ReadLE32(&sector[offset + 12]);

            if (m_partition.startLBA == 0 ||
                m_partition.totalSectors == 0)
            {
                continue;
            }

            const uint64_t start =
                m_partition.startLBA;

            const uint64_t length =
                m_partition.totalSectors;

            if (start > m_image.GetSize() / 512)
                continue;

            const uint64_t available =
                m_image.GetSize() / 512 - start;

            if (length > available)
                continue;

            m_partition.valid = true;

            break;
        }

        if (!m_partition.valid)
        {
            std::cerr
                << "ERROR: Valid FAT32 MBR partition not found.\n";

            return false;
        }

        return true;
    }

    bool BPBParser::ParseBPB()
    {
        std::vector<Byte> sector;

        if (m_image.ReadSector(
            m_partition.startLBA,
            512,
            sector
        ) != ReadStatus::Success)
        {
            std::cerr << "ERROR: Cannot read FAT32 boot sector.\n";
            return false;
        }

        if (sector.size() != 512)
            return false;

        const Byte* b = sector.data();

        if (b[510] != 0x55 || b[511] != 0xAA)
        {
            std::cerr << "ERROR: Invalid boot sector signature.\n";
            return false;
        }

        m_info.partitionStart =
            m_partition.startLBA;

        m_info.partitionSectors =
            m_partition.totalSectors;

        m_info.bytesPerSector =
            ReadLE16(&b[0x0B]);

        m_info.sectorsPerCluster =
            b[0x0D];

        m_info.reservedSectors =
            ReadLE16(&b[0x0E]);

        m_info.numberOfFATs =
            b[0x10];

        const uint16_t rootEntryCount =
            ReadLE16(&b[0x11]);

        const uint16_t totalSectors16 =
            ReadLE16(&b[0x13]);

        const uint16_t fatSize16 =
            ReadLE16(&b[0x16]);

        const uint32_t totalSectors32 =
            ReadLE32(&b[0x20]);

        const uint32_t fatSize32 =
            ReadLE32(&b[0x24]);

        m_info.extFlags =
            ReadLE16(&b[0x28]);

        m_info.fsVersion =
            ReadLE16(&b[0x2A]);

        m_info.rootCluster =
            ReadLE32(&b[0x2C]);

        m_info.fsInfoSector =
            ReadLE16(&b[0x30]);

        m_info.backupBootSector =
            ReadLE16(&b[0x32]);

        // FAT32 구조 필드
        if (rootEntryCount != 0 ||
            fatSize16 != 0 ||
            fatSize32 == 0 ||
            totalSectors16 != 0)
        {
            std::cerr
                << "ERROR: BPB does not describe a standard FAT32 volume.\n";

            return false;
        }

        m_info.totalSectors = totalSectors32;
        m_info.fatSize = fatSize32;

        return true;
    }

    bool BPBParser::ValidateBPB()
    {
        const auto fail = [](const char* message)
        {
            std::cerr << "BPB ERROR: " << message << '\n';
            return false;
        };

        const uint32_t bps = m_info.bytesPerSector;
        const uint32_t spc = m_info.sectorsPerCluster;

        // Sector size
        if (bps != 512 && bps != 1024 &&
            bps != 2048 && bps != 4096)
        {
            return fail("Unsupported bytes per sector.");
        }

        // Cluster size
        if (spc == 0 || (spc & (spc - 1)) != 0)
            return fail("Invalid sectors per cluster.");

        if (static_cast<uint64_t>(bps) * spc > 32768)
            return fail("Cluster size exceeds FAT32 limit.");

        if (m_info.reservedSectors == 0)
            return fail("Reserved sectors is zero.");

        if (m_info.numberOfFATs == 0)
            return fail("Number of FATs is zero.");

        if (m_info.fsVersion != 0)
            return fail("Unsupported FAT32 filesystem version.");

        if (m_info.rootCluster < 2)
            return fail("Invalid root cluster.");

        if (m_info.totalSectors == 0 ||
            m_info.fatSize == 0)
            return fail("Invalid total sectors or FAT size.");

        // BPB total sectors must fit inside MBR partition
        if (m_info.totalSectors > m_info.partitionSectors)
            return fail("BPB volume exceeds MBR partition.");

        const uint64_t fatArea =
            static_cast<uint64_t>(m_info.numberOfFATs) *
            m_info.fatSize;

        const uint64_t overhead =
            static_cast<uint64_t>(m_info.reservedSectors) +
            fatArea;

        if (overhead >= m_info.totalSectors)
            return fail("Invalid data area boundary.");

        m_info.clusterSize =
            static_cast<uint64_t>(bps) * spc;

        // FAT positions: absolute image byte offsets
        const uint64_t partitionByteOffset =
            static_cast<uint64_t>(m_info.partitionStart) *
            m_info.bytesPerSector;

        m_info.fat1Start =
            partitionByteOffset +
            static_cast<uint64_t>(m_info.reservedSectors) *
            m_info.bytesPerSector;

        m_info.fat2Start =
            m_info.fat1Start +
            static_cast<uint64_t>(m_info.fatSize) *
            m_info.bytesPerSector;

        m_info.firstDataSector =
            static_cast<uint64_t>(m_info.partitionStart) +
            overhead;

        m_info.dataSectors =
            m_info.totalSectors - overhead;

        m_info.clusterCount =
            m_info.dataSectors / spc;

        if (m_info.clusterCount < 65525)
            return fail("Cluster count is below FAT32 minimum.");

        if (m_info.clusterCount >
            static_cast<uint64_t>(
                std::numeric_limits<uint32_t>::max() - 1))
        {
            return fail("Cluster count exceeds supported range.");
        }

        m_info.maximumCluster =
            static_cast<uint32_t>(
                m_info.clusterCount + 1
            );

        // FAT entry capacity
        const uint64_t fatCapacity =
            static_cast<uint64_t>(m_info.fatSize) *
            bps / 4;

        if (fatCapacity <= m_info.maximumCluster)
            return fail("FAT capacity is insufficient.");

        if (m_info.rootCluster > m_info.maximumCluster)
            return fail("Root cluster exceeds data cluster range.");

        // FAT mirror configuration
        if ((m_info.extFlags & 0x0080) != 0)
        {
            const uint32_t activeFAT =
                m_info.extFlags & 0x000F;

            if (activeFAT >= m_info.numberOfFATs)
                return fail("Invalid active FAT index.");
        }

        // Verify complete filesystem byte range
        const uint64_t volumeStart =
            static_cast<uint64_t>(m_info.partitionStart) * bps;

        if (m_info.totalSectors >
            (m_image.GetSize() / bps) -
            m_info.partitionStart)
        {
            return fail("Filesystem exceeds image boundary.");
        }

        (void)volumeStart;

        return true;
    }

    const PartitionInfo& BPBParser::GetPartition() const
    {
        return m_partition;
    }

    const FAT32Info& BPBParser::GetInfo() const
    {
        return m_info;
    }

    void BPBParser::PrintReport() const
    {
        if (!m_info.valid)
        {
            std::cout << "BPB information is not validated.\n";
            return;
        }

        std::cout
            << "\n================================\n"
            << " FAT32 BPB Analysis\n"
            << "================================\n";

        std::cout
            << "Partition Start : "
            << m_info.partitionStart << '\n';

        std::cout
            << "Partition Sectors: "
            << m_info.partitionSectors << '\n';

        std::cout
            << "Bytes Per Sector: "
            << m_info.bytesPerSector << '\n';

        std::cout
            << "Sectors/Cluster : "
            << static_cast<unsigned>(
                m_info.sectorsPerCluster) << '\n';

        std::cout
            << "Cluster Size    : "
            << m_info.clusterSize << '\n';

        std::cout
            << "Reserved Sectors: "
            << m_info.reservedSectors << '\n';

        std::cout
            << "Number of FATs  : "
            << static_cast<unsigned>(
                m_info.numberOfFATs) << '\n';

        std::cout
            << "FAT Size        : "
            << m_info.fatSize << '\n';

        std::cout
            << "Total Sectors   : "
            << m_info.totalSectors << '\n';

        std::cout
            << "Root Cluster    : "
            << m_info.rootCluster << '\n';

        std::cout
            << "FAT1 Start      : "
            << m_info.fat1Start << '\n';

        std::cout
            << "FAT2 Start      : "
            << m_info.fat2Start << '\n';

        std::cout
            << "Data Start      : "
            << m_info.firstDataSector << '\n';

        std::cout
            << "Cluster Count   : "
            << m_info.clusterCount << '\n';

        std::cout
            << "Maximum Cluster : "
            << m_info.maximumCluster << '\n';

        std::cout
            << "BPB validation: PASSED\n";
    }

}

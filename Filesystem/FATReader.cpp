
#include "FATReader.h"

namespace FAT32Recovery
{
    FATReader::FATReader(
        DiskImage& image,
        const FAT32Info& info
    )
        : m_image(image),
          m_info(info)
    {
    }

    FATEntryType FATReader::GetEntryType(
        uint32_t value
    ) const
    {
        value &= 0x0FFFFFFF;

        if (value == 0)
            return FATEntryType::Free;

        if (value == 1)
            return FATEntryType::Reserved;

        if (value >= 0x0FFFFFF8)
            return FATEntryType::EndOfChain;

        if (value == 0x0FFFFFF7)
            return FATEntryType::BadCluster;

        if (value >= 0x0FFFFFF0)
            return FATEntryType::Reserved;

        if (value >= 2 && value <= m_info.maximumCluster)
            return FATEntryType::NextCluster;

        return FATEntryType::Invalid;
    }

    const char* FATReader::GetTypeName(
        FATEntryType type
    )
    {
        switch (type)
        {
            case FATEntryType::Free:
                return "FREE";

            case FATEntryType::Reserved:
                return "RESERVED";

            case FATEntryType::NextCluster:
                return "NEXT_CLUSTER";

            case FATEntryType::BadCluster:
                return "BAD_CLUSTER";

            case FATEntryType::EndOfChain:
                return "END_OF_CHAIN";

            default:
                return "INVALID";
        }
    }

    bool FATReader::ReadEntry(
        uint32_t cluster,
        uint64_t fatStart,
        FATEntry& entry
    )
    {
        entry = {};

        if (!m_info.valid)
            return false;

        if (cluster > m_info.maximumCluster)
            return false;

        // FAT32 엔트리는 4바이트
        const uint64_t entryOffset =
            static_cast<uint64_t>(cluster) * 4;

        const uint64_t fatSizeBytes =
            static_cast<uint64_t>(m_info.fatSize) *
            m_info.bytesPerSector;

        if (fatSizeBytes < 4 ||
            entryOffset > fatSizeBytes - 4)
        {
            return false;
        }

        if (fatStart >
            UINT64_MAX - entryOffset)
            return false;

        const uint64_t absoluteOffset =
            fatStart + entryOffset;

        std::vector<Byte> data;

        if (m_image.ReadBytes(
            absoluteOffset,
            4,
            data
        ) != ReadStatus::Success)
        {
            return false;
        }

        const uint32_t rawValue =
            static_cast<uint32_t>(data[0]) |
            (static_cast<uint32_t>(data[1]) << 8) |
            (static_cast<uint32_t>(data[2]) << 16) |
            (static_cast<uint32_t>(data[3]) << 24);

        const uint32_t value =
            rawValue & 0x0FFFFFFF;

        entry.cluster = cluster;

        entry.rawValue = rawValue;

        entry.value = value;

        entry.imageOffset = absoluteOffset;

        entry.type = GetEntryType(value);

        entry.valid =
            entry.type != FATEntryType::Invalid;

        // 데이터 읽기 성공 여부를 반환
        return true;
    }

    bool FATReader::ReadFAT1(
        uint32_t cluster,
        FATEntry& entry
    )
    {
        return ReadEntry(
            cluster,
            m_info.fat1Start,
            entry
        );
    }

    bool FATReader::ReadFAT2(
        uint32_t cluster,
        FATEntry& entry
    )
    {
        return ReadEntry(
            cluster,
            m_info.fat2Start,
            entry
        );
    }

    bool FATReader::CompareEntry(
        uint32_t cluster,
        FATComparison& result
    )
    {
        result = {};

        if (!ReadFAT1(cluster, result.fat1))
            return false;

        if (!ReadFAT2(cluster, result.fat2))
            return false;

        result.cluster = cluster;

        result.match =
            result.fat1.value == result.fat2.value;

        return true;
    }

    uint32_t FATReader::GetMaximumCluster() const
    {
        return m_info.maximumCluster;
    }
}


#include "DirectoryScanner.h"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace FAT32Recovery
{
    DirectoryScanner::DirectoryScanner(
        DiskImage& image
    )
        : m_image(image)
    {
    }

    bool DirectoryScanner::ParseEntry(
        const std::vector<Byte>& data,
        uint64_t offset,
        DirectoryEntry& entry
    )
    {
        if (data.size() != 32)
            return false;

        entry = {};

        entry.entryOffset = offset;
        entry.rawData = data;

        const uint8_t firstByte = data[0];
        const uint8_t attributes = data[11];

        // 디렉터리 종료 표시
        if (firstByte == 0x00)
        {
            entry.type =
                DirectoryEntryType::EndOfDirectory;

            return true;
        }

        // LFN 엔트리
        if (attributes == 0x0F)
        {
            entry.type =
                DirectoryEntryType::LFN;

            return true;
        }

        // 삭제 엔트리
        entry.deleted = (firstByte == 0xE5);

        // 속성
        entry.attributes = attributes;

        entry.isDirectory =
            (attributes & 0x10) != 0;

        entry.isVolumeLabel =
            (attributes & 0x08) != 0;

        if (entry.isVolumeLabel)
        {
            entry.type =
                DirectoryEntryType::VolumeLabel;

            return true;
        }

        // SFN 파일명
        std::string name;
        std::string extension;

        for (int i = 0; i < 8; ++i)
        {
            char c = static_cast<char>(data[i]);

            if (c == ' ')
                break;

            if (i == 0 && entry.deleted)
                c = '?';

            name += c;
        }

        for (int i = 8; i < 11; ++i)
        {
            char c = static_cast<char>(data[i]);

            if (c == ' ')
                break;

            extension += c;
        }

        entry.shortName = name;

        if (!extension.empty())
            entry.shortName += "." + extension;

        // 시작 클러스터
        entry.firstClusterHigh =
            static_cast<uint16_t>(
                data[20] |
                (data[21] << 8)
            );

        entry.firstClusterLow =
            static_cast<uint16_t>(
                data[26] |
                (data[27] << 8)
            );

        entry.startCluster =
            (static_cast<uint32_t>(
                entry.firstClusterHigh
            ) << 16) |
            entry.firstClusterLow;

        entry.startCluster &= 0x0FFFFFFF;

        // 파일 크기
        entry.fileSize =
            static_cast<uint32_t>(data[28]) |
            (static_cast<uint32_t>(data[29]) << 8) |
            (static_cast<uint32_t>(data[30]) << 16) |
            (static_cast<uint32_t>(data[31]) << 24);

        entry.type = entry.deleted
            ? DirectoryEntryType::Deleted
            : DirectoryEntryType::SFN;

        return true;
    }

    bool DirectoryScanner::ScanDirectory(
        uint64_t offset,
        uint64_t size
    )
    {
        m_entries.clear();

        if (size == 0 || size % 32 != 0)
            return false;

        if (!m_image.IsValidRange(offset, size))
            return false;

        for (uint64_t position = 0;
             position < size;
             position += 32)
        {
            std::vector<Byte> data;

            if (m_image.ReadBytes(
                    offset + position,
                    32,
                    data) != ReadStatus::Success)
            {
                return false;
            }

            DirectoryEntry entry;

            if (!ParseEntry(
                    data,
                    offset + position,
                    entry))
            {
                return false;
            }

            if (entry.type ==
                DirectoryEntryType::EndOfDirectory)
            {
                break;
            }

            if (entry.type ==
                DirectoryEntryType::Invalid)
            {
                continue;
            }

            m_entries.push_back(entry);
        }

        return true;
    }

    const std::vector<DirectoryEntry>&
    DirectoryScanner::GetEntries() const
    {
        return m_entries;
    }

    void DirectoryScanner::PrintEntries() const
    {
        std::cout
            << "\n================================\n"
            << " DIRECTORY ENTRIES\n"
            << "================================\n";

        for (const auto& entry : m_entries)
        {
            std::cout
                << "Offset: 0x"
                << std::hex
                << entry.entryOffset
                << std::dec
                << " | ";

            if (entry.type ==
                DirectoryEntryType::LFN)
            {
                std::cout << "LFN";
            }
            else
            {
                std::cout << entry.shortName;
            }

            std::cout
                << " | Cluster: "
                << entry.startCluster
                << " | Size: "
                << entry.fileSize
                << " | Deleted: "
                << (entry.deleted ? "YES" : "NO")
                << '\n';
        }
    }
}

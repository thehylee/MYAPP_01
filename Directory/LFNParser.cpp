
#include "LFNParser.h"

#include <algorithm>

namespace FAT32Recovery
{
    uint8_t LFNParser::CalculateChecksum(
        const uint8_t* shortName
    )
    {
        uint8_t checksum = 0;

        for (int i = 0; i < 11; ++i)
        {
            checksum =
                static_cast<uint8_t>(
                    ((checksum & 1) ? 0x80 : 0) +
                    (checksum >> 1) +
                    shortName[i]
                );
        }

        return checksum;
    }

    bool LFNParser::ExtractCharacters(
        const DirectoryEntry& entry,
        std::vector<uint16_t>& output
    ) const
    {
        if (entry.rawData.size() != 32)
            return false;

        const auto& d = entry.rawData;

        const int offsets[] =
        {
            1, 3, 5, 7, 9,
            14, 16, 18, 20, 22, 24,
            28, 30
        };

        output.clear();

        for (int offset : offsets)
        {
            uint16_t character =
                static_cast<uint16_t>(
                    d[offset] |
                    (static_cast<uint16_t>(
                        d[offset + 1]) << 8)
                );

            output.push_back(character);
        }

        return true;
    }

    std::string LFNParser::UTF16ToUTF8(
        const std::vector<uint16_t>& characters
    )
    {
        std::string result;

        for (size_t i = 0;
             i < characters.size();
             ++i)
        {
            uint32_t code = characters[i];

            if (code == 0x0000)
                break;

            if (code == 0xFFFF)
                continue;

            // UTF-16 surrogate pair
            if (code >= 0xD800 && code <= 0xDBFF)
            {
                if (i + 1 >= characters.size())
                    continue;

                uint32_t low = characters[++i];

                if (low < 0xDC00 || low > 0xDFFF)
                    continue;

                code =
                    0x10000 +
                    ((code - 0xD800) << 10) +
                    (low - 0xDC00);
            }
            else if (code >= 0xDC00 && code <= 0xDFFF)
            {
                continue;
            }

            if (code <= 0x7F)
            {
                result.push_back(
                    static_cast<char>(code)
                );
            }
            else if (code <= 0x7FF)
            {
                result.push_back(
                    static_cast<char>(
                        0xC0 | (code >> 6)
                    )
                );

                result.push_back(
                    static_cast<char>(
                        0x80 | (code & 0x3F)
                    )
                );
            }
            else if (code <= 0xFFFF)
            {
                result.push_back(
                    static_cast<char>(
                        0xE0 | (code >> 12)
                    )
                );

                result.push_back(
                    static_cast<char>(
                        0x80 | ((code >> 6) & 0x3F)
                    )
                );

                result.push_back(
                    static_cast<char>(
                        0x80 | (code & 0x3F)
                    )
                );
            }
            else
            {
                result.push_back(
                    static_cast<char>(
                        0xF0 | (code >> 18)
                    )
                );

                result.push_back(
                    static_cast<char>(
                        0x80 | ((code >> 12) & 0x3F)
                    )
                );

                result.push_back(
                    static_cast<char>(
                        0x80 | ((code >> 6) & 0x3F)
                    )
                );

                result.push_back(
                    static_cast<char>(
                        0x80 | (code & 0x3F)
                    )
                );
            }
        }

        return result;
    }

        
    LFNResult LFNParser::Parse(
        const std::vector<DirectoryEntry>& entries,
        size_t sfnIndex
    ) const
    {
        LFNResult result;

        if (sfnIndex >= entries.size())
            return result;

        const DirectoryEntry& sfn = entries[sfnIndex];

        if (sfn.rawData.size() != 32 ||
            sfn.type != DirectoryEntryType::SFN)
        {
            return result;
        }

        const uint8_t expectedChecksum =
            CalculateChecksum(sfn.rawData.data());

        std::vector<std::vector<uint16_t>> parts;

        uint8_t expectedSequence = 1;
        bool foundLast = false;

        size_t index = sfnIndex;

        while (index > 0)
        {
            --index;

            const DirectoryEntry& entry = entries[index];

            if (entry.type != DirectoryEntryType::LFN)
                break;

            const auto& d = entry.rawData;

            if (d.size() != 32)
                break;

            const uint8_t sequence = d[0];
            const uint8_t sequenceNumber = sequence & 0x1F;

            const bool isLast = (sequence & 0x40) != 0;

            if ((sequence & 0x80) != 0 ||
                sequenceNumber == 0 ||
                d[11] != 0x0F ||
                d[12] != 0 ||
                d[26] != 0 ||
                d[27] != 0 ||
                d[13] != expectedChecksum)
            {
                break;
            }

            if (sequenceNumber != expectedSequence)
                break;

            if (isLast)
            {
                if (foundLast)
                    break;

                foundLast = true;
            }

            std::vector<uint16_t> characters;

            if (!ExtractCharacters(entry, characters))
                break;

            // 역순으로 발견하므로 앞쪽에 삽입
            parts.insert(
                parts.begin(),
                characters
            );

            if (result.lastEntryOffset == 0)
                result.lastEntryOffset = entry.entryOffset;

            result.firstEntryOffset = entry.entryOffset;

            if (expectedSequence == 20)
                break;

            ++expectedSequence;

            if (foundLast)
            {
                // 마지막 엔트리까지 확인했으므로 종료
                break;
            }
        }

        if (!foundLast || parts.empty())
            return result;

        // 첫 번째 발견 엔트리가 0x01이어야 하며
        // 마지막에 발견한 엔트리가 마지막 번호여야 함
        if (parts.size() != expectedSequence)
            return result;

        std::vector<uint16_t> allCharacters;

        for (const auto& part : parts)
        {
            allCharacters.insert(
                allCharacters.end(),
                part.begin(),
                part.end()
            );
        }

        result.longName = UTF16ToUTF8(allCharacters);

        result.checksumValid = true;
        result.sequenceValid = true;
        result.valid = !result.longName.empty();

        return result;
    }

}

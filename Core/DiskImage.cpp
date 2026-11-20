
#include "DiskImage.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <limits>
#include <utility>

namespace FAT32Recovery
{

    DiskImage::DiskImage()
    {
    }

    DiskImage::~DiskImage()
    {
        Close();
    }

    bool DiskImage::Open(const std::string& filePath)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_file.is_open())
        {
            m_file.close();
        }

        m_info = {};

        m_file.clear();

        m_file.open(
            filePath,
            std::ios::binary | std::ios::in
        );

        if (!m_file.is_open())
        {
            return false;
        }

        m_file.seekg(0, std::ios::end);

        if (!m_file)
        {
            m_file.close();
            m_file.clear();
            return false;
        }

        const std::streampos position = m_file.tellg();

        if (position == std::streampos(-1))
        {
            m_file.close();
            m_file.clear();
            return false;
        }

        const std::streamoff size =
            static_cast<std::streamoff>(position);

        if (size < 0)
        {
            m_file.close();
            m_file.clear();
            return false;
        }

        m_info.filePath = filePath;

        m_info.imageSize =
            static_cast<uint64_t>(size);

        m_info.isOpen = true;

        m_file.clear();
        m_file.seekg(0, std::ios::beg);

        if (!m_file)
        {
            m_file.close();
            m_file.clear();
            m_info = {};
            return false;
        }

        return true;
    }

    void DiskImage::Close()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_file.is_open())
        {
            m_file.close();
        }

        m_file.clear();

        m_info = {};
    }

    bool DiskImage::IsOpen() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        return m_file.is_open() && m_info.isOpen;
    }

    uint64_t DiskImage::GetSize() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        return m_info.imageSize;
    }

    DiskImageInfo DiskImage::GetInfo() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        return m_info;
    }

    bool DiskImage::IsValidRangeUnlocked(
        uint64_t offset,
        uint64_t length
    ) const
    {
        if (!m_file.is_open() || !m_info.isOpen)
        {
            return false;
        }

        if (offset > m_info.imageSize)
        {
            return false;
        }

        // 덧셈 오버플로 방지
        if (length > m_info.imageSize - offset)
        {
            return false;
        }

        return true;
    }

    bool DiskImage::IsValidRange(
        uint64_t offset,
        uint64_t length
    ) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        return IsValidRangeUnlocked(offset, length);
    }

    ReadStatus DiskImage::ReadBytes(
        uint64_t offset,
        uint64_t length,
        std::vector<Byte>& output
    )
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        output.clear();

        if (!m_file.is_open() || !m_info.isOpen)
        {
            return ReadStatus::NotOpen;
        }

        if (length == 0)
        {
            return ReadStatus::InvalidArgument;
        }

        if (!IsValidRangeUnlocked(offset, length))
        {
            return ReadStatus::OutOfRange;
        }

        if (length >
            static_cast<uint64_t>(
                std::numeric_limits<size_t>::max()))
        {
            return ReadStatus::InvalidArgument;
        }

        if (length >
            static_cast<uint64_t>(
                std::numeric_limits<std::streamsize>::max()))
        {
            return ReadStatus::InvalidArgument;
        }

        if (offset >
            static_cast<uint64_t>(
                std::numeric_limits<std::streamoff>::max()))
        {
            return ReadStatus::InvalidArgument;
        }

        m_file.clear();

        m_file.seekg(
            static_cast<std::streamoff>(offset),
            std::ios::beg
        );

        if (!m_file)
        {
            return ReadStatus::ReadError;
        }

        std::vector<Byte> buffer(
            static_cast<size_t>(length)
        );

        m_file.read(
            reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(length)
        );

        if (m_file.gcount() !=
            static_cast<std::streamsize>(length))
        {
            return ReadStatus::ReadError;
        }

        if (!m_file)
        {
            return ReadStatus::ReadError;
        }

        output = std::move(buffer);

        return ReadStatus::Success;
    }

    ReadStatus DiskImage::ReadSector(
        uint64_t sectorNumber,
        uint32_t bytesPerSector,
        std::vector<Byte>& output
    )
    {
        output.clear();

        if (bytesPerSector == 0)
        {
            return ReadStatus::InvalidArgument;
        }

        if (sectorNumber >
            std::numeric_limits<uint64_t>::max()
            / bytesPerSector)
        {
            return ReadStatus::InvalidArgument;
        }

        const uint64_t offset =
            sectorNumber * bytesPerSector;

        return ReadBytes(
            offset,
            bytesPerSector,
            output
        );
    }

    std::string DiskImage::HexDump(
        uint64_t offset,
        uint64_t length,
        const HexDumpOptions& options
    )
    {
        if (options.bytesPerLine == 0)
        {
            return "ERROR: bytesPerLine cannot be zero";
        }

        if (length == 0)
        {
            return "ERROR: length cannot be zero";
        }

        std::vector<Byte> data;

        const ReadStatus status =
            ReadBytes(offset, length, data);

        if (status != ReadStatus::Success)
        {
            return "ERROR: Unable to read image data";
        }

        std::ostringstream stream;

        for (size_t i = 0; i < data.size();)
        {
            const size_t lineStart = i;

            const size_t remaining =
                data.size() - i;

            const size_t lineLength =
                static_cast<size_t>(
                    std::min<uint64_t>(
                        remaining,
                        options.bytesPerLine
                    )
                );

            if (options.showOffset)
            {
                stream
                    << std::hex
                    << std::uppercase
                    << std::setw(16)
                    << std::setfill('0')
                    << (offset + static_cast<uint64_t>(i))
                    << "  ";
            }

            for (size_t j = 0; j < lineLength; ++j)
            {
                stream
                    << std::hex
                    << std::uppercase
                    << std::setw(2)
                    << std::setfill('0')
                    << static_cast<unsigned int>(
                        data[i + j]
                    )
                    << ' ';
            }

            if (options.showAscii)
            {
                if (lineLength < options.bytesPerLine)
                {
                    stream
                        << std::string(
                            static_cast<size_t>(
                                options.bytesPerLine -
                                lineLength
                            ) * 3,
                            ' '
                        );
                }

                stream << " |";

                for (size_t j = 0; j < lineLength; ++j)
                {
                    const Byte value =
                        data[lineStart + j];

                    if (value >= 0x20 && value <= 0x7E)
                    {
                        stream << static_cast<char>(value);
                    }
                    else
                    {
                        stream << '.';
                    }
                }

                stream << '|';
            }

            stream << '\n';

            i += lineLength;
        }

        return stream.str();
    }

}

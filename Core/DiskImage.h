
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

namespace FAT32Recovery
{
    // 기본 자료형
    using Byte = uint8_t;
    using ByteOffset = uint64_t;

    // 읽기 결과
    enum class ReadStatus
    {
        Success,
        NotOpen,
        InvalidArgument,
        OutOfRange,
        ReadError
    };

    // 이미지 정보
    struct DiskImageInfo
    {
        std::string filePath;

        uint64_t imageSize = 0;

        bool isOpen = false;
    };

    // HEX 덤프 설정
    struct HexDumpOptions
    {
        uint64_t bytesPerLine = 16;

        bool showOffset = true;

        bool showAscii = true;
    };

    // BIN 이미지 관리
    class DiskImage
    {
    public:

        DiskImage();

        ~DiskImage();

        // 복사 금지
        DiskImage(const DiskImage&) = delete;

        DiskImage& operator=(
            const DiskImage&
        ) = delete;

        // 이동 금지
        DiskImage(DiskImage&&) = delete;

        DiskImage& operator=(
            DiskImage&&
        ) = delete;

        // 이미지 열기 (읽기 전용)
        bool Open(const std::string& filePath);

        // 이미지 닫기
        void Close();

        // 열림 여부
        bool IsOpen() const;

        // 이미지 크기
        uint64_t GetSize() const;

        // 이미지 정보
        DiskImageInfo GetInfo() const;

        // 읽기 범위 검증
        bool IsValidRange(
            uint64_t offset,
            uint64_t length
        ) const;

        // 바이트 단위 읽기
        ReadStatus ReadBytes(
            uint64_t offset,
            uint64_t length,
            std::vector<Byte>& output
        );

        // 섹터 단위 읽기
        ReadStatus ReadSector(
            uint64_t sectorNumber,
            uint32_t bytesPerSector,
            std::vector<Byte>& output
        );

        // HEX 덤프
        std::string HexDump(
            uint64_t offset,
            uint64_t length,
            const HexDumpOptions& options = {}
        );

    private:

        mutable std::mutex m_mutex;

        std::ifstream m_file;

        DiskImageInfo m_info;

        // 내부 범위 검증
        bool IsValidRangeUnlocked(
            uint64_t offset,
            uint64_t length
        ) const;
    };
}

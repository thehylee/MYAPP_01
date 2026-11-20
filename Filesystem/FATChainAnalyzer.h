
#pragma once

#include "FATReader.h"

#include <cstdint>
#include <vector>

namespace FAT32Recovery
{
    // FAT 체인 종료 상태
    enum class FATChainStatus
    {
        Complete,           // 정상적인 End Of Chain
        FreeCluster,        // 다음 클러스터가 Free
        BadCluster,         // Bad Cluster 발견
        ReservedCluster,    // Reserved 엔트리 발견
        InvalidCluster,     // 잘못된 클러스터 번호
        ReadError,          // FAT 읽기 실패
        CycleDetected,      // 순환 참조 발견
        MaximumDepthReached // 최대 추적 횟수 도달
    };

    // FAT 종류
    enum class FATSource
    {
        FAT1,
        FAT2
    };

    // 개별 FAT 체인 분석 결과
    struct FATChainResult
    {
        uint32_t startCluster = 0;

        uint32_t lastCluster = 0;

        uint64_t chainLength = 0;

        FATSource source = FATSource::FAT1;

        FATChainStatus status =
            FATChainStatus::InvalidCluster;

        // 실제로 확인된 클러스터 목록
        std::vector<uint32_t> clusters;

        // 정상적인 EOC 도달 여부
        bool isComplete = false;

        // 순환 참조 여부
        bool hasCycle = false;

        // 잘못된 엔트리 여부
        bool hasInvalidEntry = false;

        // FAT 읽기 실패 여부
        bool hasReadError = false;
    };

    // FAT1 / FAT2 체인 비교 결과
    struct FATChainComparison
    {
        FATChainResult fat1;

        FATChainResult fat2;

        bool identical = false;

        bool bothComplete = false;
    };

    class FATChainAnalyzer
    {
    public:

        explicit FATChainAnalyzer(
            FATReader& reader
        );

        // FAT1 체인 분석
        bool AnalyzeFAT1(
            uint32_t startCluster,
            FATChainResult& result
        );

        // FAT2 체인 분석
        bool AnalyzeFAT2(
            uint32_t startCluster,
            FATChainResult& result
        );

        // FAT1 / FAT2 체인 비교
        bool CompareChains(
            uint32_t startCluster,
            FATChainComparison& result
        );

        // 체인 상태 이름
        static const char* GetStatusName(
            FATChainStatus status
        );

        // FAT 종류 이름
        static const char* GetSourceName(
            FATSource source
        );

    private:

        FATReader& m_reader;

        // 공통 체인 분석
        bool AnalyzeChain(
            uint32_t startCluster,
            FATSource source,
            FATChainResult& result
        );

        // 체인 동일 여부 검사
        bool IsSameChain(
            const FATChainResult& first,
            const FATChainResult& second
        ) const;
    };
}

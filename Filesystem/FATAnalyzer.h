
#pragma once

#include "FATReader.h"

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>


namespace FAT32Recovery
{
    // FAT 종류별 엔트리 통계
    struct FATStatistics
    {
        uint64_t totalEntries = 0;

        uint64_t freeClusters = 0;
        uint64_t reservedClusters = 0;
        uint64_t nextClusters = 0;
        uint64_t badClusters = 0;
        uint64_t endOfChain = 0;
        uint64_t invalidEntries = 0;
    };

    // FAT1 / FAT2 불일치 정보
    struct FATMismatch
    {
        uint32_t cluster = 0;

        FATEntry fat1;
        FATEntry fat2;
    };

    // 유효하지 않은 엔트리 정보
    struct FATInvalidEntry
    {
        uint32_t cluster = 0;

        FATEntry entry;
        uint8_t fatNumber = 0;
    };

    // 전체 분석 결과
    struct FATAnalysisResult
    {
        bool completed = false;

        uint64_t analyzedClusters = 0;

        FATStatistics fat1Stats;
        FATStatistics fat2Stats;

        uint64_t matchingEntries = 0;
        uint64_t mismatchingEntries = 0;

        std::vector<FATMismatch> mismatches;

        std::vector<FATInvalidEntry> invalidEntries;
    };

    struct FATChainCandidate
    {
        uint32_t startCluster = 0;
        uint32_t endCluster = 0;

        uint64_t clusterCount = 0;

        bool reachesEOC = false;
        bool hasInvalidEntry = false;
        bool hasCycle = false;
        bool readError = false;
    };

    struct FATReverseAnalysisResult
    {
        uint64_t totalNextEntries = 0;
        uint64_t totalEOCEntries = 0;

        uint64_t startCandidates = 0;
        uint64_t isolatedClusters = 0;
        uint64_t referencedClusters = 0;

        std::vector<FATChainCandidate> candidates;
    };

    class FATAnalyzer
    {
    public:

        explicit FATAnalyzer(
            FATReader& reader
        );

        // 전체 FAT 분석
        bool Analyze();

        // 분석 결과 반환
        const FATAnalysisResult& GetResult() const;

        // 분석 결과 출력
        void PrintReport() const;

        FATReverseAnalysisResult AnalyzeFAT2Reverse(
            uint32_t minimumCluster,
            uint32_t maximumCluster
        );

    private:

        FATReader& m_reader;

        FATAnalysisResult m_result;

        // 통계 누적
        void UpdateStatistics(
            FATStatistics& stats,
            const FATEntry& entry
        );

        // 잘못된 엔트리 기록
        void RecordInvalidEntry(
            uint32_t cluster,
            uint8_t fatNumber,
            const FATEntry& entry
        );
    };
}

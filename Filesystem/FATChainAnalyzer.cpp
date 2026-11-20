
#include "FATChainAnalyzer.h"

#include <unordered_set>

namespace FAT32Recovery
{
    FATChainAnalyzer::FATChainAnalyzer(
        FATReader& reader
    )
        : m_reader(reader)
    {
    }

    const char* FATChainAnalyzer::GetStatusName(
        FATChainStatus status
    )
    {
        switch (status)
        {
            case FATChainStatus::Complete:
                return "COMPLETE";

            case FATChainStatus::FreeCluster:
                return "FREE_CLUSTER";

            case FATChainStatus::BadCluster:
                return "BAD_CLUSTER";

            case FATChainStatus::ReservedCluster:
                return "RESERVED_CLUSTER";

            case FATChainStatus::InvalidCluster:
                return "INVALID_CLUSTER";

            case FATChainStatus::ReadError:
                return "READ_ERROR";

            case FATChainStatus::CycleDetected:
                return "CYCLE_DETECTED";

            case FATChainStatus::MaximumDepthReached:
                return "MAXIMUM_DEPTH_REACHED";

            default:
                return "UNKNOWN";
        }
    }

    const char* FATChainAnalyzer::GetSourceName(
        FATSource source
    )
    {
        switch (source)
        {
            case FATSource::FAT1:
                return "FAT1";

            case FATSource::FAT2:
                return "FAT2";

            default:
                return "UNKNOWN";
        }
    }

    bool FATChainAnalyzer::AnalyzeFAT1(
        uint32_t startCluster,
        FATChainResult& result
    )
    {
        return AnalyzeChain(
            startCluster,
            FATSource::FAT1,
            result
        );
    }

    bool FATChainAnalyzer::AnalyzeFAT2(
        uint32_t startCluster,
        FATChainResult& result
    )
    {
        return AnalyzeChain(
            startCluster,
            FATSource::FAT2,
            result
        );
    }

    bool FATChainAnalyzer::AnalyzeChain(
        uint32_t startCluster,
        FATSource source,
        FATChainResult& result
    )
    {
        result = {};

        result.startCluster = startCluster;
        result.source = source;

        const uint32_t maximumCluster =
            m_reader.GetMaximumCluster();

        // 시작 클러스터 검증
        if (startCluster < 2 ||
            startCluster > maximumCluster)
        {
            result.status =
                FATChainStatus::InvalidCluster;

            result.hasInvalidEntry = true;

            return false;
        }

        // 이미 방문한 클러스터 관리
        std::unordered_set<uint32_t> visited;

        // FAT1 또는 FAT2 엔트리 읽기
        auto readEntry = [&](
            uint32_t cluster,
            FATEntry& entry
        ) -> bool
        {
            if (source == FATSource::FAT1)
            {
                return m_reader.ReadFAT1(
                    cluster,
                    entry
                );
            }

            return m_reader.ReadFAT2(
                cluster,
                entry
            );
        };

        uint32_t currentCluster = startCluster;

        while (true)
        {
            // 최대 클러스터 범위 검사
            if (currentCluster < 2 ||
                currentCluster > maximumCluster)
            {
                result.status =
                    FATChainStatus::InvalidCluster;

                result.hasInvalidEntry = true;

                return true;
            }

            // 순환 참조 검사
            if (!visited.insert(currentCluster).second)
            {
                result.status =
                    FATChainStatus::CycleDetected;

                result.hasCycle = true;

                return true;
            }

            // FAT 엔트리 읽기
            FATEntry entry;

            if (!readEntry(
                currentCluster,
                entry))
            {
                result.status =
                    FATChainStatus::ReadError;

                result.hasReadError = true;

                return true;
            }

            // 실제로 확인한 클러스터 기록
            result.clusters.push_back(
                currentCluster
            );

            result.lastCluster =
                currentCluster;

            result.chainLength =
                result.clusters.size();

            // 엔트리 유효성 검사
            if (!entry.valid)
            {
                result.status =
                    FATChainStatus::InvalidCluster;

                result.hasInvalidEntry = true;

                return true;
            }

            switch (entry.type)
            {
                case FATEntryType::Free:

                    result.status =
                        FATChainStatus::FreeCluster;

                    return true;

                case FATEntryType::Reserved:

                    result.status =
                        FATChainStatus::ReservedCluster;

                    return true;

                case FATEntryType::BadCluster:

                    result.status =
                        FATChainStatus::BadCluster;

                    return true;

                case FATEntryType::EndOfChain:

                    result.status =
                        FATChainStatus::Complete;

                    result.isComplete = true;

                    return true;

                case FATEntryType::NextCluster:

                    currentCluster =
                        entry.value;

                    break;

                case FATEntryType::Invalid:

                default:

                    result.status =
                        FATChainStatus::InvalidCluster;

                    result.hasInvalidEntry = true;

                    return true;
            }

            // 방문 클러스터가 최대 개수를 초과하지 않도록 제한
            if (result.chainLength >=
                static_cast<uint64_t>(maximumCluster) - 1)
            {
                result.status =
                    FATChainStatus::MaximumDepthReached;

                return true;
            }
        }
    }

    bool FATChainAnalyzer::IsSameChain(
        const FATChainResult& first,
        const FATChainResult& second
    ) const
    {
        if (first.clusters.size() !=
            second.clusters.size())
        {
            return false;
        }

        for (size_t i = 0;
             i < first.clusters.size();
             ++i)
        {
            if (first.clusters[i] !=
                second.clusters[i])
            {
                return false;
            }
        }

        return true;
    }

    bool FATChainAnalyzer::CompareChains(
        uint32_t startCluster,
        FATChainComparison& result
    )
    {
        result = {};

        const bool fat1Success =
            AnalyzeFAT1(
                startCluster,
                result.fat1
            );

        const bool fat2Success =
            AnalyzeFAT2(
                startCluster,
                result.fat2
            );

        if (!fat1Success || !fat2Success)
            return false;

        result.identical =
            IsSameChain(
                result.fat1,
                result.fat2
            );

        result.bothComplete =
            result.fat1.isComplete &&
            result.fat2.isComplete;

        return true;
    }
}

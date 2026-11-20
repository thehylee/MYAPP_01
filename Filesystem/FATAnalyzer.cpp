
#include "FATAnalyzer.h"

#include <iostream>
#include <iomanip>

namespace FAT32Recovery
{
    FATAnalyzer::FATAnalyzer(
        FATReader& reader
    )
        : m_reader(reader)
    {
    }

    void FATAnalyzer::UpdateStatistics(
        FATStatistics& stats,
        const FATEntry& entry
    )
    {
        ++stats.totalEntries;

        switch (entry.type)
        {
            case FATEntryType::Free:
                ++stats.freeClusters;
                break;

            case FATEntryType::Reserved:
                ++stats.reservedClusters;
                break;

            case FATEntryType::NextCluster:
                ++stats.nextClusters;
                break;

            case FATEntryType::BadCluster:
                ++stats.badClusters;
                break;

            case FATEntryType::EndOfChain:
                ++stats.endOfChain;
                break;

            case FATEntryType::Invalid:
                ++stats.invalidEntries;
                break;
        }
    }

    void FATAnalyzer::RecordInvalidEntry(
        uint32_t cluster,
        uint8_t fatNumber,
        const FATEntry& entry
    )
    {
        FATInvalidEntry invalid;

        invalid.cluster = cluster;
        invalid.fatNumber = fatNumber;
        invalid.entry = entry;

        m_result.invalidEntries.push_back(invalid);
    }

    bool FATAnalyzer::Analyze()
    {
        m_result = {};

        const uint32_t maximumCluster =
            m_reader.GetMaximumCluster();

        if (maximumCluster < 2)
            return false;

        std::cout
            << "\nStarting FAT analysis...\n";

        for (uint32_t cluster = 2;
             cluster <= maximumCluster;
             ++cluster)
        {
            FATComparison comparison;

            if (!m_reader.CompareEntry(
                    cluster,
                    comparison))
            {
                // 읽기 실패는 Invalid와 구분해야 하므로
                // 현재 분석은 실패 처리한다.
                std::cerr
                    << "FAT read failed at cluster: "
                    << cluster << '\n';

                return false;
            }

            UpdateStatistics(
                m_result.fat1Stats,
                comparison.fat1
            );

            UpdateStatistics(
                m_result.fat2Stats,
                comparison.fat2
            );

            if (!comparison.fat1.valid)
            {
                RecordInvalidEntry(
                    cluster, 1, comparison.fat1
                );
            }

            if (!comparison.fat2.valid)
            {
                RecordInvalidEntry(
                    cluster, 2, comparison.fat2
                );
            }

            if (comparison.match)
            {
                ++m_result.matchingEntries;
            }
            else
            {
                ++m_result.mismatchingEntries;

                FATMismatch mismatch;

                mismatch.cluster = cluster;
                mismatch.fat1 = comparison.fat1;
                mismatch.fat2 = comparison.fat2;

                m_result.mismatches.push_back(mismatch);
            }

            ++m_result.analyzedClusters;
        }

        m_result.completed = true;

        return true;
    }

    const FATAnalysisResult&
    FATAnalyzer::GetResult() const
    {
        return m_result;
    }

    void FATAnalyzer::PrintReport() const
    {
        if (!m_result.completed)
        {
            std::cout
                << "FAT analysis not completed.\n";

            return;
        }

        auto printStats = [](
            const char* title,
            const FATStatistics& stats)
        {
            std::cout
                << "\n[" << title << "]\n";

            std::cout
                << "Total Entries : "
                << stats.totalEntries << '\n';

            std::cout
                << "Free Clusters : "
                << stats.freeClusters << '\n';

            std::cout
                << "Reserved      : "
                << stats.reservedClusters << '\n';

            std::cout
                << "Next Cluster  : "
                << stats.nextClusters << '\n';

            std::cout
                << "Bad Cluster   : "
                << stats.badClusters << '\n';

            std::cout
                << "End Of Chain  : "
                << stats.endOfChain << '\n';

            std::cout
                << "Invalid       : "
                << stats.invalidEntries << '\n';
        };

        std::cout
            << "\n================================\n"
            << " FAT ANALYSIS REPORT\n"
            << "================================\n";

        std::cout
            << "Analyzed Clusters : "
            << m_result.analyzedClusters << '\n';

        printStats(
            "FAT1 Statistics",
            m_result.fat1Stats
        );

        printStats(
            "FAT2 Statistics",
            m_result.fat2Stats
        );

        std::cout
            << "\n[FAT Comparison]\n";

        std::cout
            << "Matching Entries    : "
            << m_result.matchingEntries << '\n';

        std::cout
            << "Mismatching Entries : "
            << m_result.mismatchingEntries << '\n';

        std::cout
            << "Invalid Entries     : "
            << m_result.invalidEntries.size() << '\n';
    }
}


#pragma once

#include "DirectoryScanner.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace FAT32Recovery
{

    enum class TreeNodeType
    {
        Root,
        Directory,
        File,
        DeletedFile,
        DeletedDirectory,
        VolumeLabel
    };

    struct TreeNode
    {
        uint64_t entryOffset = 0;

        TreeNodeType type = TreeNodeType::File;

        std::string name;
        std::string shortName;
        std::string fullPath;

        uint32_t startCluster = 0;
        uint32_t fileSize = 0;

        uint8_t attributes = 0;

        bool deleted = false;
        bool isDirectory = false;

        TreeNode* parent = nullptr;

        bool isDeleted = false;
        bool isVolumeLabel = false;
        bool hasValidLFN = false;

        std::vector<std::unique_ptr<TreeNode>> children;

        TreeNode* AddChild(
            std::unique_ptr<TreeNode> child
        );
    };

    struct TreeStatistics
    {
        uint64_t totalFiles = 0;
        uint64_t totalDirectories = 0;
        uint64_t totalDeletedFiles = 0;
        uint64_t totalDeletedDirectories = 0;
        uint64_t totalBytes = 0;
    };

    class TreeBuilder
    {
    public:

        TreeBuilder();

        // 루트 노드 초기화
        void InitializeRoot(
            uint32_t rootCluster
        );

        // 트리 초기화
        void Clear();

        // 디렉터리 엔트리 등록
        bool AddDirectoryEntries(
            TreeNode* parent,
            const std::vector<DirectoryEntry>& entries
        );

        // 자식 노드 추가
        TreeNode* AddChild(
            TreeNode* parent,
            const DirectoryEntry& entry
        );

        // 루트 반환
        TreeNode* GetRoot() const;

        // 클러스터로 노드 검색
        TreeNode* FindNode(
            uint32_t cluster
        ) const;

        // 이름으로 검색
        TreeNode* FindByName(
            const std::string& name
        ) const;

        // 전체 경로 반환
        std::string GetFullPath(
            const TreeNode* node
        ) const;

        // 파일 목록
        std::vector<const TreeNode*> GetFiles() const;

        // 디렉터리 목록
        std::vector<const TreeNode*> GetDirectories() const;

        // 트리 출력
        void PrintTree() const;

        // 통계
        TreeStatistics GetStatistics() const;

        // 전체 노드 목록
        std::vector<TreeNode*> GetAllNodes() const;

        // 클러스터로 노드 검색
        TreeNode* FindByCluster(
            uint32_t cluster
        ) const;

        // 전체 경로 생성
        std::string BuildFullPath(
            const TreeNode* node
        ) const;

        // 노드 개수
        size_t GetNodeCount() const;

    private:

        std::unique_ptr<TreeNode> m_root;

        // 내부 재귀 함수
        void PrintNode(
            const TreeNode* node,
            size_t depth
        ) const;

        void CollectNodes(
            const TreeNode* node,
            std::vector<const TreeNode*>& files,
            std::vector<const TreeNode*>& directories
        ) const;

        void CollectStatistics(
            const TreeNode* node,
            TreeStatistics& statistics
        ) const;

        TreeNode* FindNodeRecursive(
            TreeNode* node,
            uint32_t cluster
        ) const;

        TreeNode* FindByNameRecursive(
            TreeNode* node,
            const std::string& name
        ) const;

    };

}

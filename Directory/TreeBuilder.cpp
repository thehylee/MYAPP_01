
#include "TreeBuilder.h"

#include <iostream>
#include <utility>

namespace FAT32Recovery
{

    // ========================================
    // TreeNode
    // ========================================

    TreeNode* TreeNode::AddChild(
        std::unique_ptr<TreeNode> child
    )
    {
        if (!child)
            return nullptr;

        child->parent = this;

        TreeNode* result = child.get();

        children.push_back(std::move(child));

        return result;
    }

    // ========================================
    // Constructor
    // ========================================

    TreeBuilder::TreeBuilder()
    {
    }

    // ========================================
    // InitializeRoot
    // ========================================

    void TreeBuilder::InitializeRoot(
        uint32_t rootCluster
    )
    {
        Clear();

        m_root = std::make_unique<TreeNode>();

        m_root->type = TreeNodeType::Root;
        m_root->name = "/";
        m_root->shortName = "/";
        m_root->startCluster = rootCluster;
        m_root->isDirectory = true;
    }

    // ========================================
    // Clear
    // ========================================

    void TreeBuilder::Clear()
    {
        m_root.reset();
    }

    // ========================================
    // AddDirectoryEntries
    // ========================================

    bool TreeBuilder::AddDirectoryEntries(
        TreeNode* parent,
        const std::vector<DirectoryEntry>& entries
    )
    {
        if (!parent)
            return false;

        for (const auto& entry : entries)
        {
            // 디렉터리 종료 엔트리 제외
            if (entry.type == DirectoryEntryType::EndOfDirectory)
                continue;

            // 무효 엔트리 제외
            if (entry.type == DirectoryEntryType::Invalid)
                continue;

            // LFN은 별도 LFNParser에서 처리
            if (entry.type == DirectoryEntryType::LFN)
                continue;

            AddChild(parent, entry);
        }

        return true;
    }

    // ========================================
    // AddChild
    // ========================================

    TreeNode* TreeBuilder::AddChild(
        TreeNode* parent,
        const DirectoryEntry& entry
    )
    {
        if (!parent)
            return nullptr;

        auto node = std::make_unique<TreeNode>();
        node->parent = parent;

        node->entryOffset = entry.entryOffset;

        node->name = entry.shortName;
        node->shortName = entry.shortName;

        node->isDeleted = entry.deleted;
        node->isVolumeLabel = entry.isVolumeLabel;

        node->startCluster = entry.startCluster;
        node->fileSize = entry.fileSize;

        node->attributes = entry.attributes;

        node->deleted = entry.deleted;
        node->isDirectory = entry.isDirectory;

        if (entry.isVolumeLabel)
        {
            node->type = TreeNodeType::VolumeLabel;
        }
        else if (entry.deleted)
        {
            node->type =
                entry.isDirectory
                ? TreeNodeType::DeletedDirectory
                : TreeNodeType::DeletedFile;
        }
        else
        {
            node->type =
                entry.isDirectory
                ? TreeNodeType::Directory
                : TreeNodeType::File;
        }

        return parent->AddChild(std::move(node));
    }

    // ========================================
    // GetRoot
    // ========================================

    TreeNode* TreeBuilder::GetRoot() const
    {
        return m_root.get();
    }

    // ========================================
    // FindNode
    // ========================================

    TreeNode* TreeBuilder::FindNode(
        uint32_t cluster
    ) const
    {
        return FindNodeRecursive(
            m_root.get(),
            cluster
        );
    }

    TreeNode* TreeBuilder::FindNodeRecursive(
        TreeNode* node,
        uint32_t cluster
    ) const
    {
        if (!node)
            return nullptr;

        if (node->startCluster == cluster)
            return node;

        for (const auto& child : node->children)
        {
            TreeNode* result =
                FindNodeRecursive(
                    child.get(),
                    cluster
                );

            if (result)
                return result;
        }

        return nullptr;
    }

    // ========================================
    // FindByName
    // ========================================

    TreeNode* TreeBuilder::FindByName(
        const std::string& name
    ) const
    {
        for (TreeNode* node : GetAllNodes())
        {
            if (node->name == name ||
                node->shortName == name)
            {
                return node;
            }
        }

        return nullptr;
    }

    TreeNode* TreeBuilder::FindByNameRecursive(
        TreeNode* node,
        const std::string& name
    ) const
    {
        if (!node)
            return nullptr;

        if (node->name == name)
            return node;

        for (const auto& child : node->children)
        {
            TreeNode* result =
                FindByNameRecursive(
                    child.get(),
                    name
                );

            if (result)
                return result;
        }

        return nullptr;
    }

    // ========================================
    // GetFullPath
    // ========================================

    std::string TreeBuilder::GetFullPath(
        const TreeNode* node
    ) const
    {
        if (!node)
            return "";

        if (node == m_root.get())
            return "/";

        std::vector<std::string> parts;

        const TreeNode* current = node;

        while (current && current != m_root.get())
        {
            parts.push_back(current->name);
            current = current->parent;
        }

        std::string path = "/";

        for (auto it = parts.rbegin();
            it != parts.rend();
            ++it)
        {
            path += *it;

            if (it + 1 != parts.rend())
                path += "/";
        }

        return path;
    }

    // ========================================
    // CollectNodes
    // ========================================

    void TreeBuilder::CollectNodes(
        const TreeNode* node,
        std::vector<const TreeNode*>& files,
        std::vector<const TreeNode*>& directories
    ) const
    {
        if (!node)
            return;

        for (const auto& child : node->children)
        {
            if (child->isDirectory)
                directories.push_back(child.get());
            else if (child->type != TreeNodeType::VolumeLabel)
                files.push_back(child.get());

            CollectNodes(
                child.get(),
                files,
                directories
            );
        }
    }

    // ========================================
    // GetFiles
    // ========================================

    std::vector<const TreeNode*> TreeBuilder::GetFiles() const
    {
        std::vector<const TreeNode*> files;
        std::vector<const TreeNode*> directories;

        CollectNodes(
            m_root.get(),
            files,
            directories
        );

        return files;
    }

    // ========================================
    // GetDirectories
    // ========================================

    std::vector<const TreeNode*> TreeBuilder::GetDirectories() const
    {
        std::vector<const TreeNode*> files;
        std::vector<const TreeNode*> directories;

        CollectNodes(
            m_root.get(),
            files,
            directories
        );

        return directories;
    }

    // ========================================
    // PrintNode
    // ========================================

    void TreeBuilder::PrintNode(
        const TreeNode* node,
        size_t depth
    ) const
    {
        if (!node)
            return;

        for (const auto& child : node->children)
        {
            for (size_t i = 0; i < depth; ++i)
                std::cout << "  ";

            switch (child->type)
            {
                case TreeNodeType::Directory:
                    std::cout << "[DIR] ";
                    break;

                case TreeNodeType::File:
                    std::cout << "[FILE] ";
                    break;

                case TreeNodeType::DeletedFile:
                    std::cout << "[DELETED FILE] ";
                    break;

                case TreeNodeType::DeletedDirectory:
                    std::cout << "[DELETED DIR] ";
                    break;

                case TreeNodeType::VolumeLabel:
                    std::cout << "[VOLUME] ";
                    break;

                default:
                    std::cout << "[UNKNOWN] ";
                    break;
            }

            std::cout << child->name;

            std::cout
                << " | Cluster: "
                << child->startCluster;

            if (!child->isDirectory)
            {
                std::cout
                    << " | Size: "
                    << child->fileSize;
            }

            if (child->deleted)
                std::cout << " | Deleted";

            std::cout << '\n';

            PrintNode(
                child.get(),
                depth + 1
            );
        }
    }

    // ========================================
    // PrintTree
    // ========================================

    void TreeBuilder::PrintTree() const
    {
        if (!m_root)
        {
            std::cout << "Tree is empty.\n";
            return;
        }

        std::cout << "\n================================\n";
        std::cout << " DIRECTORY TREE\n";
        std::cout << "================================\n";

        std::cout
            << "[ROOT] /"
            << " | Cluster: "
            << m_root->startCluster
            << '\n';

        PrintNode(m_root.get(), 1);

        TreeStatistics stats = GetStatistics();

        std::cout << "\nFiles       : "
                << stats.totalFiles << '\n';

        std::cout << "Directories : "
                << stats.totalDirectories << '\n';

        std::cout << "Deleted Files: "
                << stats.totalDeletedFiles << '\n';

        std::cout << "Deleted Directories: "
                << stats.totalDeletedDirectories << '\n';

        std::cout << "Total Bytes : "
                << stats.totalBytes << '\n';
    }

    // ========================================
    // Statistics
    // ========================================

    void TreeBuilder::CollectStatistics(
        const TreeNode* node,
        TreeStatistics& statistics
    ) const
    {
        if (!node)
            return;

        for (const auto& child : node->children)
        {
            switch (child->type)
            {
                case TreeNodeType::Directory:
                    ++statistics.totalDirectories;
                    break;

                case TreeNodeType::File:
                    ++statistics.totalFiles;
                    statistics.totalBytes += child->fileSize;
                    break;

                case TreeNodeType::DeletedFile:
                    ++statistics.totalDeletedFiles;
                    statistics.totalBytes += child->fileSize;
                    break;

                case TreeNodeType::DeletedDirectory:
                    ++statistics.totalDeletedDirectories;
                    break;

                default:
                    break;
            }

            CollectStatistics(
                child.get(),
                statistics
            );
        }
    }

    // ========================================
    // GetStatistics
    // ========================================

    TreeStatistics TreeBuilder::GetStatistics() const
    {
        TreeStatistics statistics;

        CollectStatistics(
            m_root.get(),
            statistics
        );

        return statistics;
    }

    std::vector<TreeNode*> TreeBuilder::GetAllNodes() const
    {
        std::vector<TreeNode*> result;

        if (!m_root)
            return result;

        std::vector<TreeNode*> stack;

        stack.push_back(m_root.get());

        while (!stack.empty())
        {
            TreeNode* node = stack.back();
            stack.pop_back();

            result.push_back(node);

            for (auto it = node->children.rbegin();
                it != node->children.rend();
                ++it)
            {
                stack.push_back(it->get());
            }
        }

        return result;
    }

    TreeNode* TreeBuilder::FindByCluster(
        uint32_t cluster
    ) const
    {
        for (TreeNode* node : GetAllNodes())
        {
            if (node->startCluster == cluster)
                return node;
        }

        return nullptr;
    }

    std::string TreeBuilder::BuildFullPath(
        const TreeNode* node
    ) const
    {
        if (!node)
            return "";

        std::vector<std::string> parts;

        while (node && node != m_root.get())
        {
            parts.push_back(node->name);
            node = node->parent;
        }

        std::string path = "/";

        for (auto it = parts.rbegin();
            it != parts.rend();
            ++it)
        {
            path += *it;

            if (std::next(it) != parts.rend())
                path += "/";
        }

        return path;
    }

    size_t TreeBuilder::GetNodeCount() const
    {
        return GetAllNodes().size();
    }
}
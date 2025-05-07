#pragma once

#include <vector>
#include <memory>
#include <filesystem>
#include <string>
#include <choc/platform/choc_DynamicLibrary.h>

class NodeManager {
public:
    ~NodeManager();

    // Loads all dynamic node objects from the given folder (e.g., "objects/")
    void loadAll(const std::filesystem::path& objectDir);

    // Optional: unloads all node libraries
    void unloadAll();

private:
    std::vector<std::unique_ptr<choc::file::DynamicLibrary>> loadedObjects;
};
#pragma once

#include <vector>
#include <memory>
#include <filesystem>
#include <iostream>
#include <choc/platform/choc_DynamicLibrary.h>

class NodeManager {
public:

    ~NodeManager()
    {
        unloadAll();
    }

    // Loads all dynamic node objects from the given folder (e.g., "objects/")
    void loadAll(const std::filesystem::path& objectDir);

    void unloadAll()
    {
        loadedObjects.clear();
    }

private:
    std::vector<std::unique_ptr<choc::file::DynamicLibrary>> loadedObjects;
};
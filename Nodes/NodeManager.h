#pragma once

#include <vector>
#include <memory>
#include <filesystem>
#include <iostream>
#include <choc/platform/choc_DynamicLibrary.h>

class NodeManager {
public:

    // Loads all dynamic node objects from the given folder (e.g., "objects/")
    void loadAll(const std::filesystem::path& objectDir);

private:
    std::vector<std::unique_ptr<choc::file::DynamicLibrary>> loadedObjects;
};
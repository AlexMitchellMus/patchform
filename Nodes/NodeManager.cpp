#include "NodeManager.h"
#include <iostream>

NodeManager::~NodeManager()
{
    unloadAll();
}

void NodeManager::loadAll(const std::filesystem::path& objectDir)
{
    if (!std::filesystem::exists(objectDir) || !std::filesystem::is_directory(objectDir))
    {
        std::cerr << "[NodeManager] objects folder does not exist: " << objectDir << std::endl;
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(objectDir))
    {
        if (!entry.is_regular_file())
            continue;

        auto ext = entry.path().extension().string();
#ifdef _WIN32
        if (ext != ".dll") continue;
#elif __APPLE__
        if (ext != ".dylib") continue;
#else
        if (ext != ".so") continue;
#endif

        auto lib = std::make_unique<choc::file::DynamicLibrary>(entry.path().string());
        if (!lib->handle)
        {
            std::cerr << "Failed to load node object: " << entry.path() << std::endl;
            continue;
        }

        using RegisterFunc = void(*)();
        if (auto reg = lib->findFunction("registerPatchformNodes"))
        {
            reinterpret_cast<RegisterFunc>(reg)();
            std::cout << "Registered node object: " << entry.path().filename() << std::endl;
            loadedObjects.push_back(std::move(lib)); // keep handle alive
        }
        else
        {
            std::cerr << "Missing registerPatchformNodes() in: " << entry.path() << std::endl;
        }
    }
}

void NodeManager::unloadAll()
{
    loadedObjects.clear();
}

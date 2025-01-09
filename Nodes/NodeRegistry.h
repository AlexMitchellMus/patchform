#pragma once

#include <iostream>
#include <vector>
#include <string>

class NodeRegistry {
public:
    static NodeRegistry& getInstance() {
        static NodeRegistry instance;
        return instance;
    }

    void registerNode(const std::string& name) {
        nodeNames.push_back(name);
    }

    const std::vector<std::string>& getNodeNames() const {
        return nodeNames;
    }

private:
    std::vector<std::string> nodeNames;

    NodeRegistry()
    {
        nodeNames.reserve(100);
    };
    NodeRegistry(const NodeRegistry&) = delete;
    NodeRegistry& operator=(const NodeRegistry&) = delete;
};
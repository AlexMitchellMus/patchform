#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <json.hpp>
#include "ankerl/unordered_dense.h"

using json = nlohmann::json;

class NodeRegistry {
public:
    static NodeRegistry& getInstance() {
        static NodeRegistry instance;
        return instance;
    }

    void registerNode(const std::string& name) {
        nodeNames.push_back(name);
    }

    using FactoryFn = std::function<AudioNode*(NodeContext*, const json&)>;

    void registerAlias(const std::vector<std::string>& aliases, FactoryFn fn) {
        for (const auto& alias : aliases)
            factories[alias] = fn;
    }

    const std::vector<std::string>& getNodeNames() const {
        return nodeNames;
    }

    AudioNode* createNode(const std::string& alias, NodeContext* ctx, const json& j) const {
        auto it = factories.find(alias);
        if (it != factories.end())
            return it->second(ctx, j);
        return nullptr;
    }

private:
    std::vector<std::string> nodeNames;

    ankerl::unordered_dense::map<std::string, FactoryFn> factories;

    NodeRegistry()
    {
        nodeNames.reserve(100);
    };
    NodeRegistry(const NodeRegistry&) = delete;
    NodeRegistry& operator=(const NodeRegistry&) = delete;
};
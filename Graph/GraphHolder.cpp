//
// Created by alexw on 2/05/2025.
//
#include "GraphHolder.h"
#include "GraphManager.h"

uint32_t GraphHolder::generateGlobalID() const
{
    uint32_t id = 0;

    auto root = parentGraph;
    while (root->parentGraph) root = root->parentGraph;

    while (root->usedGlobalIDs.contains(id)) ++id;

    root->usedGlobalIDs.insert(id);

    std::cout << "parent graph: " << root << " new ID is: " << id << std::endl;

    return id;
}


//
// Created by alexw on 2/05/2025.
//
#include "GraphHolder.h"
#include "GraphManager.h"

uint32_t GraphHolder::generateGlobalID() const
{
    std::set<uint32_t> usedIDs;

    std::function<void(GraphHolder*)> collectIDsFromHolder = [&](GraphHolder* holder) {
        if (!holder) return;

        for (auto* obj : holder->getObjects())
            usedIDs.insert(obj->nodeID);

        for (auto* obj : holder->getObjects()) {
            if (auto* sub = dynamic_cast<Subpatch*>(obj)) {
                if (auto* subgraph = sub->getSubgraph()) {
                    auto* nestedHolder = subgraph->transitioningGraph ? subgraph->transitioningGraph.get()
                                                                      : subgraph->activeGraph.get();
                    collectIDsFromHolder(nestedHolder);
                }
            }
        }
    };

    if (parentGraph) {
        GraphManager* root = parentGraph;
        while (root->parentGraph)
            root = root->parentGraph;

        auto* rootHolder = root->transitioningGraph ? root->transitioningGraph.get()
                                                    : root->activeGraph.get();
        collectIDsFromHolder(rootHolder);
    }

    // Always collect local
    collectIDsFromHolder(const_cast<GraphHolder*>(this));

    uint32_t idCounter = 0;
    while (usedIDs.contains(idCounter))
        ++idCounter;

    std::cout << "parent graph: " << parentGraph << " new ID is: " << idCounter << std::endl;
    return idCounter;
}


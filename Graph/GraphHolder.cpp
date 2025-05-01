//
// Created by alexw on 2/05/2025.
//
#include "GraphHolder.h"
#include "GraphManager.h"

uint32_t GraphHolder::generateGlobalID() const {
    std::set<unsigned int> usedIDs;

    // Traverse to the root graph
    GraphManager* root = parentGraph;
    while (root->parentGraph)
        root = root->parentGraph;

    // Collect all node IDs in all subgraphs
    std::function<void(GraphManager*)> collectIDs = [&](GraphManager* gm) {
        if (!gm || !gm->getActiveGraph())
            return;

        for (auto* obj : gm->getActiveGraph()->getObjects())
            usedIDs.insert(obj->nodeID);

        for (auto& obj : gm->getActiveGraph()->getObjects()) {
            if (auto* sub = dynamic_cast<Subpatch*>(obj)) {
                collectIDs(sub->getSubgraph());
            }
        }
    };
    collectIDs(root);

    uint32_t idCounter = 0;
    while (usedIDs.contains(idCounter))
        ++idCounter;

    return idCounter;
}
#pragma once

class EventPool {
public:
    EventPool(std::size_t initialSize = 1024) {
        events.reserve(initialSize);
        growPool(initialSize);
    }

    int eventPoolSize()
    {
        return events.size();
    }

    // Acquire a free Event (returns nullptr if none available)
    Event* getFreeEvent() {
        //std::cout << "freestack size: " << freeStack.size() << std::endl;
        if (freeStack.empty()) {
            std::cout << "event exhaustion, size: " << freeStack.size() << std::endl;
            // No free events left!
            // Do NOT grow here if you're in the audio callback
            // Return nullptr or handle logic as you wish
            return nullptr;
        }
        // Pop index from stack
        //std::cout << "stack index: " << freeStack.back() << std::endl;
        auto eventIndex = freeStack.back();
        freeStack.pop_back();

        Event* evt = &events[eventIndex];
        evt->setTimeStamp(0);
        return evt;
    }

    void releaseAllEvents()
    {
        freeStack = templateStack;
    }

    // Grow the pool by adding N new events (call this OUTSIDE audio callback)
    void growPool(std::size_t count) {
        // Current size
        auto oldSize = events.size();

        auto newSize = oldSize + count;
        events.resize(newSize);

        freeStack.resize(newSize);
        std::iota(freeStack.begin(), freeStack.end(), 0);

        templateStack.resize(newSize);
        std::iota(templateStack.begin(), templateStack.end(), 0);
    }

private:
    std::vector<Event> events;               // Actual storage
    std::vector<std::size_t> freeStack;      // Indices of free events
    std::vector<std::size_t> templateStack;  // Template of a full freestack
};

class NodeContext {
public:
    float sampleRate;
    int frameCount;

    EventPool eventPool;

    NodeContext(float sampleRate, int frameCount) : sampleRate(sampleRate), frameCount(frameCount) {}
};
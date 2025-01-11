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
        // Pop index from top of stack
        std::size_t eventIndex = freeStack.back();
        freeStack.pop_back();

        Event* evt = &events[eventIndex];
        evt->setTimeStamp(0);
        return evt;
    }

    // Return an Event to the pool
    void returnFreeEvent(Event* evt) {
        if (!evt) return;

        // Calculate index
        std::size_t index = static_cast<std::size_t>(evt - &events[0]);
        freeStack.push_back(index);
    }

    // Grow the pool by adding N new events (call this OUTSIDE audio callback)
    void growPool(std::size_t count) {
        // Current size
        std::size_t oldSize = events.size();

        // Increase by 'count'
        events.resize(oldSize + count);

        // Push new indices onto freeStack
        for (std::size_t i = 0; i < count; ++i) {
            freeStack.push_back(oldSize + i);
        }
    }

private:
    std::vector<Event> events;       // Actual storage
    std::vector<std::size_t> freeStack;  // Indices of free events
};

class NodeContext {
public:
    float sampleRate;
    int frameCount;

    EventPool eventPool;

    NodeContext(float sampleRate, int frameCount) : sampleRate(sampleRate), frameCount(frameCount) {}
};
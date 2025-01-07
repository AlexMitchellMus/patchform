#pragma once

#include <vector>
#include <iostream>

class Event
{
    uint64_t timeStamp = 0;
public:
    Event(){};

    Event(uint64_t timeStamp) : timeStamp(timeStamp) {}

    uint64_t getTimeStamp() const
    {
        return timeStamp;
    }

    Event& setTimeStamp(const uint64_t timestamp)
    {
        timeStamp = timestamp;
        return *this;
    }
};

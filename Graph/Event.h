#pragma once

#include <vector>
#include <iostream>

class Tag
{
public:
    std::string nameName;
    hash32 tagHash;

    Tag(const std::string& tag) : nameName(tag), tagHash(hash(tag)){}
};

class Event
{
    uint64_t timeStamp = 0;
    Tag tag = Tag("trigger");

public:
    Event(){};

    Event(uint64_t timeStamp) : timeStamp(timeStamp) {}

    void setTag(const std::string& tagName)
    {
        tag = Tag(tagName);
    }

    hash32 getTagHash()
    {
        return tag.tagHash;
    }

    uint64_t getTimeStamp() const
    {
        return timeStamp;
    }

    Event& setTimeStamp(const uint64_t timestamp)
    {
        timeStamp = timestamp;
        return *this;
    }

    float data;
};

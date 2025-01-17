/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <string>
#include <atomic>
#include <iostream>
#include <thread>
#include "concurrentqueue.h"

class AudioNode;

class Logger {
public:
    static Logger& getInstance();

    void logEvent(AudioNode* node, uint64_t timestamp, float data);

    void log(const std::string& message);

    // Start the log processing thread
    void startProcessingThread();

    // Stop the log processing thread
    void stopProcessingThread();

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void processLogs();

    struct Message
    {
        std::string nodeName;
        float data = 0.0f;
        uint64_t timestamp = 0;
    };

    moodycamel::ConcurrentQueue<Logger::Message> logQueue;
    std::atomic<bool> stopProcessing{false};
    std::thread processingThread;
};

/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Logger.h"
#include <iostream>
#include <chrono>

#include "../Nodes/AudioNodeBase.h"

// Singleton instance of Logger
Logger& Logger::getInstance() {
    std::cout << "============ logger created =============" << std::endl;
    static Logger instance;
    return instance;
}

// Object event message logging
void Logger::logEvent(AudioNode* node, uint64_t timestamp, float data) {
    logQueue.enqueue(Message{ node->getName(), data, timestamp }); // Enqueue log message safely
}

// String only message logging
void Logger::log(const std::string& message) {
    logQueue.enqueue(Message{ message });
}

// Start the log processing thread
void Logger::startProcessingThread() {
    stopProcessing = false;
    processingThread = std::thread([this]() { processLogs(); });
}

// Stop the log processing thread
void Logger::stopProcessingThread() {
    stopProcessing = true;
    if (processingThread.joinable()) {
        processingThread.join();
    }
}

// Destructor to clean up the thread
Logger::~Logger() {
    stopProcessingThread();
}

// Process logs in a non-real-time thread
void Logger::processLogs() {
    Message msg;
    while (!stopProcessing) {
        while (logQueue.try_dequeue(msg)) {
            std::cout << msg.nodeName << " " << msg.data << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Flush remaining messages
    while (logQueue.try_dequeue(msg)) {
        std::cout << msg.nodeName << " " << msg.data << std::endl;
    }
}
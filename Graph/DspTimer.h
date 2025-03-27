#pragma once

#include <atomic>
#include <chrono>

class DspTimer {
public:
    // Constructor initializes the timer
    DspTimer()
      : cpuUsage(0.0),
        lastPrintTime(std::chrono::high_resolution_clock::now()),
        accumulatedCallbackTimeMs(0.0),
        callCount(0)
    {}

    // Call at the beginning of the DSP process
    void start()
    {
        startTime = std::chrono::high_resolution_clock::now();
    }

    inline void end(unsigned long frameCount, double sampleRate)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        double callbackTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        // Accumulate callback times and increment the call counter
        accumulatedCallbackTimeMs += callbackTimeMs;
        ++callCount;

        // Check if at least one second has elapsed since last update
        auto now = std::chrono::high_resolution_clock::now();
        double elapsedSec = std::chrono::duration<double>(now - lastPrintTime).count();
        if (elapsedSec >= 1.0) {
            double averageMs = accumulatedCallbackTimeMs / callCount;
            // Calculate the available time (in ms) per callback
            double periodMs = 1000.0 * (static_cast<double>(frameCount) / sampleRate);
            double usagePct = (averageMs / periodMs) * 100.0;

            // Update the atomic CPU usage variable
            cpuUsage.store(usagePct, std::memory_order_relaxed);

            // Reset the counters for the next interval
            lastPrintTime = now;
            accumulatedCallbackTimeMs = 0.0;
            callCount = 0;
        }
    }

    // Returns the latest computed CPU usage percentage
    inline double getCpuUsage() const
    {
        return cpuUsage.load(std::memory_order_relaxed);
    }

private:
    std::atomic<double> cpuUsage; // Latest CPU usage (% of available time)
    std::chrono::high_resolution_clock::time_point lastPrintTime;
    std::chrono::high_resolution_clock::time_point startTime;
    double accumulatedCallbackTimeMs;
    int callCount;
};
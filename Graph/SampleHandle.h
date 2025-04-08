#pragma once

struct Sample {
    std::atomic<int> refCount {1};
    std::vector<float> samples;
    size_t size = 0;
    size_t numChannels = 1;
    double sampleRate = 44100.0;

    Sample(size_t numSamples = 0, size_t channels = 1)
        : samples(numSamples * channels), numChannels(channels) {}

    ~Sample() = default;
};


struct SampleHandle {
    Sample* sample = nullptr;

    SampleHandle() = default;

    explicit SampleHandle(Sample* s) : sample(s) {
        retain();
    }

    SampleHandle(const SampleHandle& other) : sample(other.sample) {
        retain();
    }

    SampleHandle& operator=(const SampleHandle& other) {
        if (this != &other) {
            release();
            sample = other.sample;
            retain();
        }
        return *this;
    }

    ~SampleHandle() {
        release();
    }

    Sample* get() const { return sample; }
    bool isValid() const { return sample != nullptr; }
    SampleHandle& makeSampleHandle();

    // Factory function
    static SampleHandle makeSampleHandle(size_t numSamples, size_t channels = 1, double sampleRate = 44100.0) {
        if (numSamples == 0)
            return SampleHandle();

        auto* s = new Sample(numSamples, channels);
        s->sampleRate = sampleRate;
        return SampleHandle(s);
    }

private:
    void retain() {
        if (sample)
            sample->refCount.fetch_add(1, std::memory_order_relaxed);
    }

    void release() {
        if (sample && sample->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete sample;
        }
        sample = nullptr;
    }
};
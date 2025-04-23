#pragma once

#include <vector>
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>

class ScopedSwapBuffer {
    static constexpr size_t NumBuffers = 4;
    static constexpr int Mask = NumBuffers - 1;

public:
    explicit ScopedSwapBuffer(size_t size = 0) {
        resize(size);
    }

    void resize(size_t size) {
        for (auto& b : buffers)
            b.resize(size);
    }

    size_t size() const {
        return buffers[0].size();
    }

    struct ScopedReadAccess {
        const float* data;
        explicit ScopedReadAccess(const float* ptr) : data(ptr) {}
        const float* operator->() const { return data; }
        const float& operator[](size_t i) const { return data[i]; }
    };

    struct ScopedWriteAccess {
        float* data;
        std::function<void()> commit;

        ScopedWriteAccess(float* ptr, std::function<void()> c)
            : data(ptr), commit(std::move(c)) {}

        ~ScopedWriteAccess() {
            commit();
        }

        float* operator->() { return data; }
        float& operator[](size_t i) { return data[i]; }
    };

    ScopedReadAccess acquireScopedReader() {
        trySwapToRead();
        return ScopedReadAccess(buffers[readIndex.load(std::memory_order_relaxed)].data());
    }

    ScopedWriteAccess acquireScopedWriter() {
        int w = writeIndex.load(std::memory_order_relaxed);
        float* ptr = buffers[w].data();

        return ScopedWriteAccess(ptr, [this, w] {
            int r = readIndex.load(std::memory_order_relaxed);
            middleIndex.store(w, std::memory_order_release);

            // Branchless 3-slot scan (guaranteed free slot)
            int c1 = (w + 1) & Mask;
            int c2 = (w + 2) & Mask;
            int c3 = (w + 3) & Mask;

            int next = (c1 != r && c1 != w) ? c1 :
                       (c2 != r && c2 != w) ? c2 : c3;

            writeIndex.store(next, std::memory_order_release);
        });
    }

private:
    std::array<std::vector<float>, NumBuffers> buffers;
    std::atomic<int> writeIndex{0};
    std::atomic<int> readIndex{1};
    std::atomic<int> middleIndex{2};

    void trySwapToRead() {
        int m = middleIndex.load(std::memory_order_acquire);
        int r = readIndex.load(std::memory_order_relaxed);

        if (m != r)
            readIndex.store(m, std::memory_order_release);
    }
};

#pragma once

#include <cstdlib>
#include <new>

template<typename T, std::size_t Alignment = 32>
struct AlignedAllocator {
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = AlignedAllocator<U, Alignment>;
    };

    AlignedAllocator() noexcept = default;

    template<typename U>
    AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

    T* allocate(std::size_t n) {
        void* ptr = nullptr;
#if defined(_MSC_VER)
        ptr = _aligned_malloc(n * sizeof(T), Alignment);
        if (!ptr) throw std::bad_alloc();
#elif defined(__APPLE__) || defined(__linux__)
        if (posix_memalign(&ptr, Alignment, n * sizeof(T)) != 0)
            throw std::bad_alloc();
#endif
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t) noexcept {
#if defined(_MSC_VER)
        _aligned_free(p);
#else
        free(p);
#endif
    }

    template<typename U>
    bool operator==(const AlignedAllocator<U, Alignment>&) const noexcept {
        return true;
    }

    template<typename U>
    bool operator!=(const AlignedAllocator<U, Alignment>&) const noexcept {
        return false;
    }
};

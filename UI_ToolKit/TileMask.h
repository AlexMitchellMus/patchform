#pragma once

#include <choc/containers/choc_SmallVector.h>
#include <algorithm>
#include <functional>

class TileMask {
public:
    static constexpr int tileSize = 32;

    void resize(int width, int height)
    {
        tilesX = (width + tileSize - 1) / tileSize;
        tilesY = (height + tileSize - 1) / tileSize;
        int wordCount = ((tilesX * tilesY) + 63) / 64;
        bits.resize(wordCount);
        std::ranges::fill(bits, 0);
    }

    void clear() {
        std::ranges::fill(bits, 0);
    }

    void set(int x, int y) {
        int index = y * tilesX + x;
        bits[index >> 6] |= 1ULL << (index & 63);
    }

    void setIndex(int index) {
        bits[index >> 6] |= 1ULL << (index & 63);
    }

    bool test(int x, int y) const {
        int index = y * tilesX + x;
        return (bits[index >> 6] >> (index & 63)) & 1ULL;
    }

    bool testIndex(int index) const {
        return (bits[index >> 6] >> (index & 63)) & 1ULL;
    }

    [[nodiscard]] int getX() const { return tilesX; }
    [[nodiscard]] int getY() const { return tilesY; }

    choc::SmallVector<uint64_t, 64>& raw() { return bits; }
    const choc::SmallVector<uint64_t, 64>& raw() const { return bits; }

    void forEachSetRun(std::function<void(int xStart, int xEnd, int y)> fn) const {
        for (int y = 0; y < tilesY; ++y) {
            int x = 0;
            while (x < tilesX) {
                int idx = y * tilesX + x;
                if (!testIndex(idx)) { ++x; continue; }

                int startX = x;
                while (x < tilesX && testIndex(y * tilesX + x)) ++x;

                fn(startX, x, y);
            }
        }
    }

private:
    int tilesX = -1, tilesY = -1;
    choc::SmallVector<uint64_t, 64> bits;
};

class TileMaskBuffer {
public:
    static constexpr int tileSize = 32;

    void resize(int width, int height)
    {
        current.resize(width, height);
        previous.resize(width, height);
        merged.resize(width, height);
    }

    [[nodiscard]] bool isInit() const
    {
        return current.getX() > 0 && current.getY() > 0;
    }

    [[nodiscard]] bool testTile(const int x, const int y) const
    {
        return merged.test(x, y);
    }

    void clearCurrent()
    {
        current.clear();
    }

    [[nodiscard]] int getX() const
    {
        return current.getX();
    }

    [[nodiscard]] int getY() const
    {
        return current.getY();
    }

    void mergePrevious()
    {
        auto& a = current.raw();
        auto& b = previous.raw();
        auto& out = merged.raw();

        for (size_t i = 0; i < a.size(); ++i)
            out[i] = a[i] | b[i];

        b = a;
    }

    TileMask current;
    TileMask previous;
    TileMask merged;
};

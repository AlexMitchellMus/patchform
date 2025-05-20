#pragma once

#include <choc/containers/choc_SmallVector.h>
#include <algorithm>
#include <functional>
#include <span>

class TileMask {
public:
    static constexpr int tileSize = 32;

    void resize(int pixelWidth, int pixelHeight)
    {
        const int numTilesX = (pixelWidth + tileSize - 1) / tileSize;
        const int numTilesY = (pixelHeight + tileSize - 1) / tileSize;
        resizeTiles(numTilesX, numTilesY);
    }

    void resizeTiles(int numTilesX, int numTilesY)
    {
        tilesX = numTilesX;
        tilesY = numTilesY;

        const size_t tileCount = static_cast<size_t>(tilesX) * tilesY;
        const size_t wordCount = (tileCount + 63) / 64;

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

    void setWord(const int wordIndex, const uint64_t mask)
    {
        if (wordIndex >= 0 && wordIndex < static_cast<int>(bits.size()))
            bits[wordIndex] |= mask;
    }

    void copyTo(TileMask& dest)
    {
        dest.resizeTiles(getX(), getY());
        std::ranges::copy(bits, dest.bits.begin());
    }

    [[nodiscard]] bool test(int x, int y) const
    {
        int index = y * tilesX + x;
        return (bits[index >> 6] >> (index & 63)) & 1ULL;
    }

    [[nodiscard]] bool testIndex(int index) const
    {
        return (bits[index >> 6] >> (index & 63)) & 1ULL;
    }

    [[nodiscard]] int getX() const { return tilesX; }
    [[nodiscard]] int getY() const { return tilesY; }

    std::span<uint64_t> getSpan()
    {
        return {bits.data(), bits.size()};
    }

    [[nodiscard]] std::span<const uint64_t> getSpan() const
    {
        return {bits.data(), bits.size()};
    }

    [[nodiscard]] bool hasDirtyBits()
    {
        for (const auto& word : bits) {
            if (word != 0)
                return true;
        }
        return false;
    }

    choc::SmallVector<uint64_t, 64>& raw() { return bits; }
    [[nodiscard]] const choc::SmallVector<uint64_t, 64>& raw() const { return bits; }

    void printDebug(const char* label = nullptr) const {
        if (label)
            std::cout << "=== " << label << " ===\n";
        else
            std::cout << "=== " << "tile bits"  << " ===\n";

        for (int y = 0; y < tilesY; ++y) {
            for (int x = 0; x < tilesX; ++x)
                std::cout << (test(x, y) ? "█ " : "░ ");
            std::cout << "\n";
        }
    }

private:
    int tilesX = -1, tilesY = -1;
    choc::SmallVector<uint64_t, 64> bits;
};

class TileMaskBuffer {
public:
    static constexpr int tileSize = 32;

    void resizeFromWidth(int width, int height)
    {
        currentTileMask.resize(width, height);
        previousTileMask.resize(width, height);
        mergedTileMask.resize(width, height);
    }

    void resize(int tileX, int tileY)
    {
        currentTileMask.resizeTiles(tileX, tileY);
        previousTileMask.resizeTiles(tileX, tileY);
        mergedTileMask.resizeTiles(tileX, tileY);
    }

    [[nodiscard]] bool isInit() const
    {
        return currentTileMask.getX() > 0 && currentTileMask.getY() > 0;
    }

    [[nodiscard]] bool testTile(const int x, const int y) const
    {
        return mergedTileMask.test(x, y);
    }

    [[nodiscard]] int getX() const
    {
        return currentTileMask.getX();
    }

    [[nodiscard]] int getY() const
    {
        return currentTileMask.getY();
    }

    void mergeInto(TileMask& mask)
    {
        auto& cur = currentTileMask.raw();
        auto& prev = previousTileMask.raw();
        auto& merged = mergedTileMask.raw();
        auto& out = mask.raw();

        for (size_t i = 0; i < cur.size(); ++i)
        {
            merged[i] = cur[i] | prev[i];
            prev[i] = cur[i];
            out[i] |= merged[i];
        }
    }

    TileMask currentTileMask;
    TileMask previousTileMask;
    TileMask mergedTileMask;
};

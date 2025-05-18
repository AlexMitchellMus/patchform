#include <choc/containers/choc_SmallVector.h>
#include <algorithm>
#include <functional>

class TileMask {
public:
    static constexpr int tileSize = 32;

    void resize(int width, int height) {
        tilesX = (width + tileSize - 1) / tileSize;
        tilesY = (height + tileSize - 1) / tileSize;
        int wordCount = ((tilesX * tilesY) + 63) / 64;
        currentDirtyTiles.resize(wordCount, 0);
        previousDirtyTiles.resize(wordCount, 0);
        mergedTiles.resize(wordCount, 0);
    }

    void clear() {
        std::ranges::fill(currentDirtyTiles, 0);
    }

    void setTile(int x, int y) {
        int index = y * tilesX + x;
        currentDirtyTiles[index / 64] |= 1ULL << (index % 64);
    }

    void mergePrevious() {
        for (size_t i = 0; i < currentDirtyTiles.size(); ++i)
            mergedTiles[i] = currentDirtyTiles[i] | previousDirtyTiles[i];
        previousDirtyTiles = currentDirtyTiles;
    }

    void forEachDirtyTileRun(std::function<void(int xStart, int xEnd, int y)> fn) const {
        for (int y = 0; y < tilesY; ++y) {
            int x = 0;
            while (x < tilesX) {
                int idx = y * tilesX + x;
                if (!(mergedTiles[idx / 64] & (1ULL << (idx % 64)))) {
                    ++x;
                    continue;
                }
                int startX = x;
                while (x < tilesX) {
                    idx = y * tilesX + x;
                    if (!(mergedTiles[idx / 64] & (1ULL << (idx % 64))))
                        break;
                    ++x;
                }
                fn(startX, x, y);
            }
        }
    }

private:
    int tilesX = -1, tilesY = -1;
    choc::SmallVector<uint64_t, 64> currentDirtyTiles;
    choc::SmallVector<uint64_t, 64> previousDirtyTiles;
    choc::SmallVector<uint64_t, 64> mergedTiles;
};

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <nanovg.h>

namespace pptk
{
    // Structures for overall and per-glyph metrics.
    struct FontMetrics
    {
        float ascender;
        float descender;
        float lineHeight;
    };

    struct GlyphMetrics
    {
        float minx, miny, maxx, maxy;
        float advance; // The measured advance width.
    };

    // This class encapsulates the font metrics caching.
    class FontMetricsCache
    {
    public:
        // Cache the metrics for each font provided in fontNames.
        // Assumes an active NanoVG context (e.g. between nvgBeginFrame and nvgEndFrame).
        void cacheFontMetrics(NVGcontext* vg, const std::vector<std::string>& fontNames,
                              const std::vector<float>& fontSizes)
        {
            for (const std::string& fontName : fontNames)
            {
                std::map<float, std::unordered_map<unsigned int, GlyphMetrics>> sizeGlyphCache;
                std::map<float, std::unordered_map<unsigned long long, float>> sizeKerningCache;

                for (float baseFontSize : fontSizes)
                {
                    nvgFontFace(vg, fontName.c_str());
                    nvgFontSize(vg, baseFontSize);
                    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

                    std::unordered_map<unsigned int, GlyphMetrics> glyphMetrics;
                    for (unsigned int cp = 32; cp < 256; ++cp)
                    {
                        std::string glyphStr = codepointToUTF8(cp);
                        float bounds[4] = {0};
                        nvgTextBounds(vg, 0, 0, glyphStr.c_str(), nullptr, bounds);

                        GlyphMetrics gm;
                        gm.minx = bounds[0];
                        gm.miny = bounds[1];
                        gm.maxx = bounds[2];
                        gm.maxy = bounds[3];
                        gm.advance = nvgTextBounds(vg, 0, 0, glyphStr.c_str(), nullptr, nullptr);

                        glyphMetrics[cp] = gm;
                    }
                    sizeGlyphCache[baseFontSize] = std::move(glyphMetrics);

                    std::unordered_map<unsigned long long, float> kernings;
                    for (unsigned int cp1 = 32; cp1 < 256; ++cp1)
                    {
                        if (sizeGlyphCache[baseFontSize].find(cp1) == sizeGlyphCache[baseFontSize].end()) continue;
                        for (unsigned int cp2 = 32; cp2 < 256; ++cp2)
                        {
                            if (sizeGlyphCache[baseFontSize].find(cp2) == sizeGlyphCache[baseFontSize].end()) continue;

                            std::string pairStr;
                            pairStr.push_back(static_cast<char>(cp1));
                            pairStr.push_back(static_cast<char>(cp2));

                            float pairWidth = nvgTextBounds(vg, 0, 0, pairStr.c_str(), nullptr, nullptr);
                            float expectedWidth = sizeGlyphCache[baseFontSize][cp1].advance + sizeGlyphCache[
                                baseFontSize][cp2].advance;
                            float kerning = pairWidth - expectedWidth;

                            unsigned long long key = (static_cast<unsigned long long>(cp1) << 32) | cp2;
                            kernings[key] = kerning;
                        }
                    }
                    sizeKerningCache[baseFontSize] = std::move(kernings);
                }

                glyphCacheMap[fontName] = std::move(sizeGlyphCache);
                kerningCacheMap[fontName] = std::move(sizeKerningCache);
            }
        }

        float getTextWidth(const std::string& fontName, float size, const std::string& text) const
        {
            if (text.empty()) return 0.0f;

            auto fontIt = glyphCacheMap.find(fontName);
            if (fontIt == glyphCacheMap.end())
            {
                std::cerr << "Font '" << fontName << "' not found in glyph cache." << std::endl;
                return 0.0f;
            }

            const auto& sizeGlyphCache = fontIt->second;
            auto kernIt = kerningCacheMap.find(fontName);
            const auto& sizeKerningCache = (kernIt != kerningCacheMap.end()
                                                ? kernIt->second
                                                : std::map<float, std::unordered_map<unsigned long long, float>>{});

            // Find closest font sizes
            auto lower = sizeGlyphCache.lower_bound(size);
            auto upper = sizeGlyphCache.upper_bound(size);

            // If size is larger than the largest cached size, use the largest available size and scale
            if (lower == sizeGlyphCache.end())
            {
                --lower;
                float maxCachedSize = lower->first;
                float scale = size / maxCachedSize; // Scale proportionally

                float scaledWidth = getTextWidth(fontName, maxCachedSize, text) * scale;
                return scaledWidth;
            }

            if (upper == sizeGlyphCache.end()) upper = lower;
            if (lower != sizeGlyphCache.begin()) --lower;

            float lowerSize = lower->first;
            float upperSize = upper->first;
            const auto& lowerGlyphMap = lower->second;
            const auto& upperGlyphMap = upper->second;
            float scaleMix = (size - lowerSize) / (upperSize - lowerSize + 1e-6f);

            float width = 0.0f;
            unsigned int prevCp = 0;
            bool first = true;

            for (char c : text)
            {
                unsigned int cp = static_cast<unsigned int>(c);
                float advance = 0.0f;

                if (lowerGlyphMap.find(cp) != lowerGlyphMap.end() && upperGlyphMap.find(cp) != upperGlyphMap.end())
                {
                    advance = lowerGlyphMap.at(cp).advance * (1.0f - scaleMix) + upperGlyphMap.at(cp).advance *
                        scaleMix;
                }
                else if (lowerGlyphMap.find(cp) != lowerGlyphMap.end())
                {
                    advance = lowerGlyphMap.at(cp).advance;
                }
                else if (upperGlyphMap.find(cp) != upperGlyphMap.end())
                {
                    advance = upperGlyphMap.at(cp).advance;
                }

                if (!first)
                {
                    unsigned long long key = (static_cast<unsigned long long>(prevCp) << 32) | cp;
                    float kerning = 0.0f;

                    auto lowerKerningIt = sizeKerningCache.find(lowerSize);
                    auto upperKerningIt = sizeKerningCache.find(upperSize);

                    if (lowerKerningIt != sizeKerningCache.end() && upperKerningIt != sizeKerningCache.end())
                    {
                        const auto& lowerKerning = lowerKerningIt->second;
                        const auto& upperKerning = upperKerningIt->second;

                        if (lowerKerning.find(key) != lowerKerning.end() && upperKerning.find(key) != upperKerning.
                            end())
                        {
                            kerning = lowerKerning.at(key) * (1.0f - scaleMix) + upperKerning.at(key) * scaleMix;
                        }
                        else if (lowerKerning.find(key) != lowerKerning.end())
                        {
                            kerning = lowerKerning.at(key);
                        }
                        else if (upperKerning.find(key) != upperKerning.end())
                        {
                            kerning = upperKerning.at(key);
                        }
                    }

                    width += kerning;
                }

                width += advance;
                prevCp = cp;
                first = false;
            }

            return width;
        }

    private:
        // Helper: Convert a Unicode codepoint to a UTF-8 encoded string.
        std::string codepointToUTF8(unsigned int cp)
        {
            std::string utf8;
            if (cp < 0x80)
            {
                utf8.push_back(static_cast<char>(cp));
            }
            else if (cp < 0x800)
            {
                utf8.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp < 0x10000)
            {
                utf8.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else
            {
                utf8.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                utf8.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            return utf8;
        }

    public:
        std::unordered_map<std::string, FontMetrics> fontCache;
        std::unordered_map<std::string, std::map<float, std::unordered_map<unsigned int, GlyphMetrics>>> glyphCacheMap;
        std::unordered_map<std::string, std::map<float, std::unordered_map<unsigned long long, float>>> kerningCacheMap;
    };
} // end namespace pptk

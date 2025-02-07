#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <nanovg.h>
#include <unordered_dense.h>

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
    private:
        static constexpr float BASE_FONT_SIZE = 1600.0f;

        // Helper: Convert a Unicode codepoint to a UTF-8 encoded string.
        static std::string codepointToUTF8(unsigned int cp)
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
        void cacheFontMetrics(NVGcontext* vg, const std::vector<std::string>& fontNames)
        {
            for (const std::string& fontName : fontNames)
            {
                nvgFontFace(vg, fontName.c_str());
                nvgFontSize(vg, BASE_FONT_SIZE);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

                // Cache glyphs at BASE_FONT_SIZE
                ankerl::unordered_dense::map<unsigned int, GlyphMetrics> glyphMetrics;
                for (unsigned int cp = 32; cp < 256; ++cp)
                {
                    std::string glyphStr = codepointToUTF8(cp);
                    float bounds[4] = {0};
                    float advance = nvgTextBounds(vg, 0, 0, glyphStr.c_str(), nullptr, bounds);

                    GlyphMetrics gm;
                    gm.minx = bounds[0];
                    gm.miny = bounds[1];
                    gm.maxx = bounds[2];
                    gm.maxy = bounds[3];
                    gm.advance = advance; // Ensure precise width calculation

                    glyphMetrics[cp] = gm;
                }
                glyphCacheMap[fontName] = std::move(glyphMetrics); // Store glyphs once at base size

                // Cache kerning pairs at BASE_FONT_SIZE
                ankerl::unordered_dense::map<unsigned long long, float> kernings;
                for (unsigned int cp1 = 32; cp1 < 256; ++cp1)
                {
                    if (glyphCacheMap[fontName].find(cp1) == glyphCacheMap[fontName].end()) continue;
                    for (unsigned int cp2 = 32; cp2 < 256; ++cp2)
                    {
                        if (glyphCacheMap[fontName].find(cp2) == glyphCacheMap[fontName].end()) continue;

                        std::string pairStr;
                        pairStr.push_back(static_cast<char>(cp1));
                        pairStr.push_back(static_cast<char>(cp2));

                        float pairWidth = nvgTextBounds(vg, 0, 0, pairStr.c_str(), nullptr, nullptr);
                        float expectedWidth = glyphCacheMap[fontName][cp1].advance + glyphCacheMap[fontName][cp2].advance;
                        float kerning = pairWidth - expectedWidth;

                        unsigned long long key = (static_cast<unsigned long long>(cp1) << 32) | cp2;
                        kernings[key] = kerning;
                    }
                }
                kerningCacheMap[fontName] = std::move(kernings); // Store kerning once at base size

                std::cout << "Cached metrics for font: " << fontName << std::endl;
            }
        }

        float getTextWidth(const std::string& fontName, float size, const std::string& text)
        {
            if (text.empty()) return 0.0f;

            // Find font in cache
            auto fontIt = glyphCacheMap.find(fontName);
            if (fontIt == glyphCacheMap.end())
            {
                std::cerr << "Font '" << fontName << "' not found in cache." << std::endl;
                return 0.0f;
            }

            const auto& glyphMap = fontIt->second; // Glyphs stored at BASE_FONT_SIZE
            float scale = size / BASE_FONT_SIZE;   // Correct scale factor

            // Find kerning map
            auto kernIt = kerningCacheMap.find(fontName);
            const auto* kerningMap = (kernIt != kerningCacheMap.end() ? &kernIt->second : nullptr);

            float width = 0.0f;
            unsigned int prevCp = 0;
            bool first = true;

            for (char c : text)
            {
                unsigned int cp = static_cast<unsigned int>(c);
                auto glyphIt = glyphMap.find(cp);
                if (glyphIt == glyphMap.end()) continue; // Skip missing glyphs

                const GlyphMetrics& gm = glyphIt->second;
                float advance = gm.advance * scale; // Correctly scaled advance width

                if (!first && kerningMap)
                {
                    unsigned long long key = (static_cast<unsigned long long>(prevCp) << 32) | cp;
                    auto kernPair = kerningMap->find(key);
                    if (kernPair != kerningMap->end())
                    {
                        width += kernPair->second * scale; // Proper kerning scale
                    }
                }

                width += advance;
                prevCp = cp;
                first = false;
            }

            return width;
        }

    public:
        ankerl::unordered_dense::map<std::string, FontMetrics> fontCache;
        ankerl::unordered_dense::map<std::string, ankerl::unordered_dense::map<unsigned int, GlyphMetrics>> glyphCacheMap;
        ankerl::unordered_dense::map<std::string, ankerl::unordered_dense::map<unsigned long long, float>> kerningCacheMap;
    };
} // end namespace pptk

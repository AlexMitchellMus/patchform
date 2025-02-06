#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <nanovg.h>

namespace pptk {

// Structures for overall and per-glyph metrics.
struct FontMetrics {
    float ascender;
    float descender;
    float lineHeight;
};

struct GlyphMetrics {
    float minx, miny, maxx, maxy;
    float advance; // The measured advance width.
};

// This class encapsulates the font metrics caching.
class FontMetricsCache {
public:
    // Cache the metrics for each font provided in fontNames.
    // Assumes an active NanoVG context (e.g. between nvgBeginFrame and nvgEndFrame).
    void cacheFontMetrics(NVGcontext* vg, const std::vector<std::string>& fontNames)
    {
        float baseFontSize = 18.0f;  // Use this as the base size for caching.

        for (const std::string& fontName : fontNames)
        {
            // Set the font state.
            nvgFontFace(vg, fontName.c_str());
            nvgFontSize(vg, baseFontSize);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

            // Retrieve and cache overall font metrics.
            float ascender, descender, lineHeight;
            nvgTextMetrics(vg, &ascender, &descender, &lineHeight);
            FontMetrics fm = { ascender, descender, lineHeight };
            fontCache[fontName] = fm;

            // Cache per-glyph metrics for the desired range (for example, codepoints 32 to 255).
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
                // Use nvgTextBounds with a null second bounds pointer to measure the advance width.
                float adv = nvgTextBounds(vg, 0, 0, glyphStr.c_str(), nullptr, nullptr);
                gm.advance = adv;
                glyphMetrics[cp] = gm;
            }
            glyphCacheMap[fontName] = glyphMetrics;

            // Cache kerning data for all pairs in the same range.
            // The key is a 64-bit value: (cp1 << 32) | cp2.
            std::unordered_map<unsigned long long, float> kernings;
            for (unsigned int cp1 = 32; cp1 < 256; ++cp1)
            {
                // Only process if cp1 is cached.
                if (glyphMetrics.find(cp1) == glyphMetrics.end()) continue;
                for (unsigned int cp2 = 32; cp2 < 256; ++cp2)
                {
                    if (glyphMetrics.find(cp2) == glyphMetrics.end()) continue;
                    // Create a two-character string.
                    std::string pairStr;
                    pairStr.push_back(static_cast<char>(cp1));
                    pairStr.push_back(static_cast<char>(cp2));
                    float pairWidth = nvgTextBounds(vg, 0, 0, pairStr.c_str(), nullptr, nullptr);
                    // Expected width if glyphs were laid out independently.
                    float expectedWidth = glyphMetrics[cp1].advance + glyphMetrics[cp2].advance;
                    // Kerning adjustment is the difference.
                    float kerning = pairWidth - expectedWidth;
                    unsigned long long key = (static_cast<unsigned long long>(cp1) << 32) | cp2;
                    kernings[key] = kerning;
                }
            }
            kerningCacheMap[fontName] = kernings;

            std::cout << "Cached metrics for font: " << fontName << std::endl;
        }
    }

    // Compute text width using cached glyph advances and kerning.
    float getTextWidth(const std::string& fontName, float size, const std::string& text) const
    {
        const float baseFontSize = 18.0f;  // The base size used when caching metrics.
        float scale = size / baseFontSize;
        float width = 0.0f;

        // Look up the glyph cache for the specified font.
        auto fontIt = glyphCacheMap.find(fontName);
        if (fontIt == glyphCacheMap.end())
        {
            std::cerr << "Font '" << fontName << "' not found in glyph cache." << std::endl;
            return 0.0f;
        }
        const auto& glyphMap = fontIt->second;

        // Look up the kerning cache for the specified font.
        auto kernIt = kerningCacheMap.find(fontName);
        const auto& kerningMap = (kernIt != kerningCacheMap.end() ? kernIt->second : std::unordered_map<unsigned long long, float>{});

        if (text.empty())
            return 0.0f;

        bool first = true;
        unsigned int prevCp = 0;
        for (char c : text)
        {
            unsigned int cp = static_cast<unsigned int>(c);
            if (first)
            {
                // For the first character, simply add its advance.
                try {
                    width += glyphMap.at(cp).advance;
                }
                catch (const std::out_of_range&) {
                    std::cerr << "Glyph for codepoint " << cp << " not found in font '" << fontName << "'." << std::endl;
                }
                first = false;
            }
            else
            {
                // For subsequent characters, add kerning adjustment from previous glyph.
                unsigned long long key = (static_cast<unsigned long long>(prevCp) << 32) | cp;
                if (kerningMap.find(key) != kerningMap.end())
                {
                    width += kerningMap.at(key);
                }
                // Then add the advance of the current glyph.
                try {
                    width += glyphMap.at(cp).advance;
                }
                catch (const std::out_of_range&) {
                    std::cerr << "Glyph for codepoint " << cp << " not found in font '" << fontName << "'." << std::endl;
                }
            }
            prevCp = cp;
        }
        return width * scale;
    }

    // (Optional) A helper function to print cached glyphs for debugging.
    void printCachedGlyphs() const {
        for (const auto& fontEntry : glyphCacheMap) {
            const std::string& fontName = fontEntry.first;
            const auto& glyphMap = fontEntry.second;
            std::cout << "Font: " << fontName << "\n";
            for (const auto& glyphEntry : glyphMap) {
                unsigned int cp = glyphEntry.first;
                const GlyphMetrics& gm = glyphEntry.second;
                std::cout << "  Codepoint: " << cp;
                if (cp >= 32 && cp <= 126)
                    std::cout << " ('" << static_cast<char>(cp) << "')";
                std::cout << "  Advance: " << gm.advance;
                std::cout << "  Bounds: (" << gm.minx << ", " << gm.miny << ", " << gm.maxx << ", " << gm.maxy << ")";
                std::cout << "\n";
            }
            std::cout << "\n";
        }
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
    std::unordered_map<std::string, std::unordered_map<unsigned int, GlyphMetrics>> glyphCacheMap;
    std::unordered_map<std::string, std::unordered_map<unsigned long long, float>> kerningCacheMap;
};

} // end namespace pptk

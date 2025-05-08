#pragma once

#include <string>
#include <vector>
#include <json.hpp>
#include "NanoVGWrapper.h"

namespace pptk
{
    using json = nlohmann::json;

    // --------------------
    // BaseTheme
    // --------------------
    struct BaseTheme {
        struct Field {
            const char* key;
            NVGcolor* target;
            const char* defaultHex;
        };

        virtual ~BaseTheme() = default;
        virtual std::vector<Field> getFields() = 0;

        static NVGcolor decodeColor(const std::string& hex) {
            if (hex.length() != 9 || hex[0] != '#')
                return nvgRGBA(255, 0, 255, 255); // fallback magenta

            uint32_t v = std::stoul(hex.substr(1), nullptr, 16);

            uint8_t a = (v >> 24) & 0xFF;
            uint8_t r = (v >> 16) & 0xFF;
            uint8_t g = (v >> 8)  & 0xFF;
            uint8_t b = v & 0xFF;

            return nvgRGBA(r, g, b, a);
        }

        void applyDefaults() {
            for (auto& f : getFields())
                *f.target = decodeColor(f.defaultHex);
        }

        void fromJson(const json& j) {
            const auto& root = j.contains("theme") ? j["theme"] : j;
            for (auto& f : getFields()) {
                std::string key = f.key;
                auto dot = key.find('.');
                if (dot == std::string::npos) continue;

                std::string section = key.substr(0, dot);
                std::string name = key.substr(dot + 1);

                if (root.contains(section) &&
                    root[section].contains(name) &&
                    root[section][name].is_string()) {
                    *f.target = decodeColor(root[section][name].get<std::string>());
                    }
            }
        }
    };

    // --------------------
    // Theme Macros
    // --------------------
#define DECLARE_THEME_FIELD(section, name, hex) NVGcolor section##_##name;
#define REGISTER_THEME_FIELD(section, name, hex)   \
out.push_back({ #section "." #name, &section##_##name, hex });

#define DEFINE_THEME(className, FIELD_MACRO)       \
struct className : public pptk::BaseTheme {        \
FIELD_MACRO(DECLARE_THEME_FIELD)                   \
std::vector<Field> getFields() override {          \
std::vector<Field> out;                            \
FIELD_MACRO(REGISTER_THEME_FIELD)                  \
return out;                                        \
}                                                  \
};

    // --------------------
    // AppTheme fields
    // --------------------
#define APP_THEME_FIELDS(X)             \
X(general, text,       "#ffdcdcdc")     \
X(general, foreground, "#ffe0e0e0")     \
X(general, border,     "#ff353535")     \
X(general, accent,     "#ff4080ff")     \
X(general, hover,      "#ff505050")     \
X(general, selected,   "#ff6060ff")     \
                                        \
X(topbar,  background, "#ff2b2b2b")     \
                                        \
X(panel,   background, "#ff212121")     \
                                        \
X(canvas,  background, "#ff1e1e1e")     \
                                        \
X(dialog,  background, "#ff2b2b2b")     \
X(dialog,  level_1,    "#ff2e2e2e")

    // --------------------
    // CanvasTheme fields
    // --------------------
#define CANVAS_THEME_FIELDS(X)          \
X(canvas, background, "#ff1e1e1e")      \
X(canvas, grid,       "#ff303030")      \
X(canvas, border,     "#ff353535")

    // --------------------
    // Theme Classes
    // --------------------
    DEFINE_THEME(AppTheme, APP_THEME_FIELDS)
    DEFINE_THEME(CanvasTheme, CANVAS_THEME_FIELDS)

    struct Theme {
        AppTheme app;
        CanvasTheme canvas;

        void applyDefaults() {
            app.applyDefaults();
            canvas.applyDefaults();
        }

        void fromJson(const pptk::json& j) {
            if (j.contains("app"))
                app.fromJson(j["app"]);
            if (j.contains("canvas"))
                canvas.fromJson(j["canvas"]);
        }
    };
}

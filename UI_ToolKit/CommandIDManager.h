#pragma once

#include <memory>
#include <string>
#include <utility>
#include "unordered_dense.h"
#include "SDL3/SDL.h"

class Command {
public:
    virtual ~Command() = default;
    virtual void invoke() = 0;
};

class CommandIDManager {
public:
    struct KeyCombo {
        SDL_Keycode key;
        SDL_Keymod mod;
        bool strict;

        bool operator==(const KeyCombo& other) const {
            return key == other.key && mod == other.mod && strict == other.strict;
        }
    };

    struct KeyComboHash {
        std::size_t operator()(const KeyCombo& kc) const {
            return std::hash<SDL_Keycode>{}(kc.key) ^
                   (std::hash<SDL_Keymod>{}(kc.mod) << 1) ^
                   (std::hash<bool>{}(kc.strict) << 2);
        }
    };

    void registerCommand(const std::string& id, std::unique_ptr<Command> cmd) {
        commands[id] = std::move(cmd);
    }

    void bindKey(SDL_Keycode key, SDL_Keymod mod, const std::string& commandID, const bool strict = false) {
        KeyCombo combo{key, normalizeMod(mod), strict};
        keyBindings[combo] = commandID;
    }

    Command* operator[](const std::string& id) {
        const auto it = commands.find(id);
        return it != commands.end() ? it->second.get() : nullptr;
    }

    void invokeByKey(const SDL_Keycode key, const SDL_Keymod mod) {
        const SDL_Keymod normMod = normalizeMod(mod);

        for (const auto& [combo, cmdID] : keyBindings) {
            if (combo.key != key)
                continue;

            const bool matches = combo.strict
                ? (normMod == combo.mod)
                : ((normMod & combo.mod) == combo.mod);

            if (matches) {
                if (auto* cmd = (*this)[cmdID])
                    cmd->invoke();
                return;
            }
        }
    }

private:
    static SDL_Keymod normalizeMod(SDL_Keymod mod) {
        SDL_Keymod out = SDL_KMOD_NONE;
        if (mod & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL))  out = static_cast<SDL_Keymod>(out | SDL_KMOD_CTRL);
        if (mod & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT)) out = static_cast<SDL_Keymod>(out | SDL_KMOD_SHIFT);
        if (mod & (SDL_KMOD_LALT | SDL_KMOD_RALT))    out = static_cast<SDL_Keymod>(out | SDL_KMOD_ALT);
        if (mod & (SDL_KMOD_LGUI | SDL_KMOD_RGUI))    out = static_cast<SDL_Keymod>(out | SDL_KMOD_GUI);
        return out;
    }

    ankerl::unordered_dense::map<std::string, std::unique_ptr<Command>> commands;
    ankerl::unordered_dense::map<KeyCombo, std::string, KeyComboHash> keyBindings;
};

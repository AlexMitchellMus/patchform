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
    using KeyCombo = std::pair<SDL_Keycode, SDL_Keymod>;

    struct pair_hash {
        std::size_t operator()(const KeyCombo& p) const {
            return std::hash<SDL_Keycode>{}(p.first) ^ (std::hash<SDL_Keymod>{}(p.second) << 1);
        }
    };

    void registerCommand(const std::string& id, std::unique_ptr<Command> cmd) {
        commands[id] = std::move(cmd);
    }

    void bindKey(KeyCombo combo, const std::string& commandID) {
        combo.second = normalizeMod(combo.second);
        keyBindings[combo] = commandID;
    }

    Command* operator[](const std::string& id) {
        const auto it = commands.find(id);
        return it != commands.end() ? it->second.get() : nullptr;
    }

    void invokeByKey(const SDL_Keycode key, const SDL_Keymod mod) {
        const SDL_Keymod normMod = normalizeMod(mod);

        for (const auto& [combo, cmdID] : keyBindings) {
            if (combo.first == key && (normMod & combo.second) == combo.second) {
                if (auto* cmd = (*this)[cmdID])
                    cmd->invoke();
                return;
            }
        }
    }

private:
    static SDL_Keymod normalizeMod(SDL_Keymod mod) {
        SDL_Keymod out = SDL_KMOD_NONE;
        if (mod & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL)) out = static_cast<SDL_Keymod>(out | SDL_KMOD_CTRL);
        if (mod & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT)) out = static_cast<SDL_Keymod>(out | SDL_KMOD_SHIFT);
        if (mod & (SDL_KMOD_LALT | SDL_KMOD_RALT)) out = static_cast<SDL_Keymod>(out | SDL_KMOD_ALT);
        return out;
    }

    ankerl::unordered_dense::map<std::string, std::unique_ptr<Command>> commands;
    ankerl::unordered_dense::map<KeyCombo, std::string, pair_hash> keyBindings;
};

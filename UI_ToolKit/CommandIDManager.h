#pragma once

#include "unordered_dense.h"

class Command {
public:
    virtual ~Command() = default;
    virtual void invoke() = 0;
};

class CommandIDManager {
public:
    ankerl::unordered_dense::map<std::string, std::unique_ptr<Command>> commands;

    void registerCommand(const std::string& id, std::unique_ptr<Command> cmd) {
        commands[id] = std::move(cmd);
    }

    Command* operator[](const std::string& id) {
        auto it = commands.find(id);
        return it != commands.end() ? it->second.get() : nullptr;
    }
};
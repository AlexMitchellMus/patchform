#pragma once

#include "json.hpp"
#include <string>

namespace JsonHelpers {

inline bool getBoolOrIntFallback(const nlohmann::json& j, const std::string& key, bool defaultValue = false) {
    if (!j.contains(key))
        return defaultValue;

    const auto& val = j[key];
    if (val.is_boolean())
        return val.get<bool>();
    if (val.is_number_integer())
        return val.get<int>() != 0;

    return defaultValue;
}

} // end namespace
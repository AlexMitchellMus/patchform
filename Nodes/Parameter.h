#include <variant>
#include <string>
#include <unordered_map>
#include <functional>
#include "concurrentqueue.h"

using ParameterValue = std::variant<int, float, bool, std::string>;

class Parameter {
    public:

    using Callback = std::function<void(const ParameterValue&)>;

    Parameter(const std::string& name, ParameterValue defaultValue, ParameterValue minVal, ParameterValue maxVal)
        : name(name)
        , value(defaultValue)
        , minValue(minVal)
        , maxValue(maxVal)
    {}


    template<typename T>
        T getValue() const {
        if (std::holds_alternative<T>(value)) {
            return std::get<T>(value);
        }
        return T(); // Return default value if type doesn't match
    }

    void setValue(ParameterValue newValue)
    {
        if (std::holds_alternative<float>(value) && std::holds_alternative<float>(newValue)) {
            value = std::clamp(std::get<float>(newValue), std::get<float>(minValue), std::get<float>(maxValue));
        }
        else if (std::holds_alternative<int>(value) && std::holds_alternative<int>(newValue)) {
            value = std::clamp(std::get<int>(newValue), std::get<int>(minValue), std::get<int>(maxValue));
        }
        else {
            value = newValue;
        }

        if (callback) callback(value);
    }

    void applyValue(ParameterValue newValue) {
        if (std::holds_alternative<float>(value) && std::holds_alternative<float>(newValue)) {
            value = std::clamp(std::get<float>(newValue), std::get<float>(minValue), std::get<float>(maxValue));
        }
        else if (std::holds_alternative<int>(value) && std::holds_alternative<int>(newValue)) {
            value = std::clamp(std::get<int>(newValue), std::get<int>(minValue), std::get<int>(maxValue));
        }
        else {
            value = newValue;
        }

        if (callback) callback(value);
    }



    void setCallback(Callback cb) { callback = cb; }

    const std::string& getName() const { return name; }

private:
    std::string name;
    ParameterValue value;
    ParameterValue minValue;
    ParameterValue maxValue;
    Callback callback;
};

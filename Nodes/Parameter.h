#include <string>
#include <functional>
#include "concurrentqueue.h"

// ==================== Parameter base ====================
class Parameter
{
public:
    std::string name;

    std::function<void(std::string)> onParameterChanged = [](std::string){};

    Parameter(const std::string& paramName) : name(paramName)
    {
    }

    const std::string& getName() { return name; };

    virtual void setFromString(const std::string& value) = 0;
    virtual std::string getAsString() const = 0;

    virtual ~Parameter() = default;
};

// ==================== Float Parameter ====================

class FloatParameter : public Parameter
{
public:
    FloatParameter(const std::string& name, float defaultValue, float minVal, float maxVal)
        : Parameter(name), value(defaultValue), minValue(minVal), maxValue(maxVal)
    {
    }

    void setValue(float newValue)
    {
        if (newValue <= maxValue && newValue >= minValue)
        {
            value = newValue;
            queue.enqueue(newValue);

            onParameterChanged(getAsString());
        }
    }

    float getValue()
    {
        float newValue;
        if (queue.try_dequeue(newValue))
        {
            value = std::clamp(newValue, minValue, maxValue);
        }
        return value;
    }

    void setFromString(const std::string& input) override {
        try {
            float newValue = std::stof(input);
            setValue(newValue);
        } catch (...) {}
    }

    std::string getAsString() const override
    {
        if (std::fabs(value - std::round(value)) < 1e-9)
        {
            return std::to_string(static_cast<int>(std::round(value)));
        }
        // Format the value with a fixed precision.
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << value;
        std::string s = oss.str();

        // Trim trailing zeros.
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        // If the last character is now a dot, remove it.
        if (!s.empty() && s.back() == '.')
        {
            s.pop_back();
        }
        return s;
    }

private:
    float value, minValue, maxValue;
    moodycamel::ConcurrentQueue<float> queue;
};

// ==================== Int Parameter ====================

class IntParameter : public Parameter
{
public:
    IntParameter(const std::string& name, int defaultValue, int minVal, int maxVal)
        : Parameter(name), value(defaultValue), minValue(minVal), maxValue(maxVal)
    {
    }

    void setValue(int newValue)
    {
        if (newValue <= maxValue && newValue >= minValue)
        {
            value = newValue;
            queue.enqueue(newValue);
            onParameterChanged(std::to_string(newValue));
        }
    }

    int getValue()
    {
        int newValue;
        if (queue.try_dequeue(newValue))
        {
            value = std::clamp(newValue, minValue, maxValue);
        }
        return value;
    }

    std::string getAsString() const override { return std::to_string(value); }

    void setFromString(const std::string& input) override {
        try {
            int newValue = std::stoi(input);
            setValue(newValue);
        } catch (...) {}
    }

private:
    int value, minValue, maxValue;
    moodycamel::ConcurrentQueue<int> queue;
};

// ==================== Bool Parameter ====================

class BoolParameter : public Parameter
{
public:
    BoolParameter(const std::string& name, bool defaultValue)
        : Parameter(name), value(defaultValue)
    {
    }

    void setValue(bool newValue)
    {
        queue.enqueue(newValue);
    }

    bool getValue()
    {
        bool newValue;
        if (queue.try_dequeue(newValue))
        {
            value = newValue;
        }
        return value;
    }

    std::string getAsString() const override { return value ? "true" : "false"; }

    void setFromString(const std::string& input) override {
        setValue(input == "true" || input == "1");
    }

private:
    bool value;
    moodycamel::ConcurrentQueue<bool> queue;
};

// ==================== String Parameter ====================

class StringParameter : public Parameter
{
public:
    StringParameter(const std::string& name, const std::string& defaultValue)
        : Parameter(name), value(defaultValue)
    {
    }

    void setValue(const std::string& newValue)
    {
        value = newValue;
        queue.enqueue(newValue);

        onParameterChanged(value);
    }

    std::string getValue()
    {
        std::string newValue;
        if (queue.try_dequeue(newValue))
        {
            value = newValue;
        }
        return value;
    }

    std::string getAsString() const override { return value; }

    void setFromString(const std::string& input) override {
        setValue(input);
    }

private:
    std::string value;
    moodycamel::ConcurrentQueue<std::string> queue;
};

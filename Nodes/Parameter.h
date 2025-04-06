#include <string>
#include <functional>
#include <variant>
#include "concurrentqueue.h"

#ifdef PATCHFORM_WITH_GUI
#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/TextEditor.h"
#include "../UI_ToolKit/ToggleSwitch.h"
#endif

class Parameter {
public:
    std::string name;

    std::function<void()> onParameterChanged = [](){};
    std::function<void()> informNodeOfChange = [](){};
    std::function<void(std::variant<int, float, std::string>)> updateNodeUI = [](std::variant<int, float, std::string>){};

    Parameter(const std::string& paramName) : name(paramName) {}

    const std::string& getName() { return name; }

    virtual void setFromString(const std::string& value) = 0;
    virtual std::string getAsString() const = 0;

#ifdef PATCHFORM_WITH_GUI
    virtual std::unique_ptr<pptk::Component> createEditorComponent() = 0;
    virtual void resizeEditorComponent(pptk::Component* c, int parentWidth, int parentHeight) {
        if (c) c->setBounds(parentWidth - 100, 4, 90, 22);
    }
#endif

    virtual ~Parameter() = default;
};

class FloatParameter : public Parameter {
public:
    FloatParameter(const std::string& name, float defaultValue, float minVal, float maxVal)
        : Parameter(name), value(defaultValue), minValue(minVal), maxValue(maxVal) {}

    void setValue(float newValue) {
        newValue = std::clamp(newValue, minValue, maxValue);
        if (std::fabs(newValue - value) > std::numeric_limits<float>::epsilon()) {
            value = newValue;
            queue.enqueue(newValue);
            onParameterChanged();
            informNodeOfChange();
            updateNodeUI(value);
        }
    }

    float getValue() {
        float newValue;
        while (queue.try_dequeue(newValue)) {
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

    std::string getAsString() const override {
        if (std::fabs(value - std::round(value)) < 1e-9)
            return std::to_string(static_cast<int>(std::round(value)));

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << value;
        std::string s = oss.str();
        s.erase(s.find_last_not_of('0') + 1);
        if (!s.empty() && s.back() == '.') s.pop_back();
        return s;
    }

#ifdef PATCHFORM_WITH_GUI
    std::unique_ptr<pptk::Component> createEditorComponent() override {
        auto editor = std::make_unique<pptk::TextEditor>(true);
        editor->setText(getAsString());
        editor->onTextReturned = [this, editorPtr = editor.get()]() {
            try {
                setFromString(editorPtr->getText());
            } catch (...) {}
        };
        return editor;
    }
#endif

private:
    float value, minValue, maxValue;
    moodycamel::ConcurrentQueue<float> queue;
};

class IntParameter : public Parameter {
public:
    IntParameter(const std::string& name, int defaultValue, int minVal, int maxVal)
        : Parameter(name), value(defaultValue), minValue(minVal), maxValue(maxVal) {}

    void setValue(int newValue) {
        newValue = std::clamp(newValue, minValue, maxValue);
        if (value != newValue) {
            value = newValue;
            queue.enqueue(newValue);
            informNodeOfChange();
            onParameterChanged();
            updateNodeUI(value);
        }
    }

    int getValue() {
        int newValue;
        while (queue.try_dequeue(newValue)) {
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

#ifdef PATCHFORM_WITH_GUI
    std::unique_ptr<pptk::Component> createEditorComponent() override {
        auto editor = std::make_unique<pptk::TextEditor>(true);
        editor->setText(getAsString());
        editor->onTextReturned = [this, editorPtr = editor.get()]() {
            try {
                setFromString(editorPtr->getText());
            } catch (...) {}
        };
        return editor;
    }
#endif

private:
    int value, minValue, maxValue;
    moodycamel::ConcurrentQueue<int> queue;
};

class BoolParameter : public Parameter {
public:
    BoolParameter(const std::string& name, bool defaultValue)
        : Parameter(name), value(defaultValue) {}

    void setValue(bool newValue) {
        queue.enqueue(newValue);
        informNodeOfChange();
    }

    bool getValue() {
        bool newValue;
        while (queue.try_dequeue(newValue)) {
            value = newValue;
        }
        return value;
    }

    std::string getAsString() const override { return value ? "true" : "false"; }

    void setFromString(const std::string& input) override {
        setValue(input == "true" || input == "1");
    }

#ifdef PATCHFORM_WITH_GUI
    std::unique_ptr<pptk::Component> createEditorComponent() override
    {
        auto toggle = std::make_unique<ToggleSwitch>();
        toggle->setState(getValue());
        toggle->onToggle = [this](bool state) {
            setValue(state);
        };
        return toggle;
    }

    void resizeEditorComponent(pptk::Component* c, int parentWidth, int parentHeight) override
    {
        if (c) c->setBounds(parentWidth - 90, 6, 33, 18);
    }
#endif

private:
    bool value;
    moodycamel::ConcurrentQueue<bool> queue;
};

class StringParameter : public Parameter {
public:
    StringParameter(const std::string& name, const std::string& defaultValue)
        : Parameter(name), value(defaultValue) {}

    void setValue(const std::string& newValue) {
        value = newValue;
        queue.enqueue(newValue);
        onParameterChanged();
        informNodeOfChange();
        updateNodeUI(value);
    }

    std::string getValue() {
        std::string newValue;
        if (queue.try_dequeue(newValue)) {
            value = newValue;
        }
        return value;
    }

    std::string getAsString() const override { return value; }

    void setFromString(const std::string& input) override {
        setValue(input);
    }

#ifdef PATCHFORM_WITH_GUI
    std::unique_ptr<pptk::Component> createEditorComponent() override {
        auto editor = std::make_unique<pptk::TextEditor>(false);
        editor->setText(getValue());
        editor->onTextReturned = [this, editorPtr = editor.get()]() {
            try {
                setFromString(editorPtr->getText());
            } catch (...) {}
        };
        return editor;
    }
#endif

private:
    std::string value;
    moodycamel::ConcurrentQueue<std::string> queue;
};
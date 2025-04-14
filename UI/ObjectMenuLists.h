//
// Created by alexw on 8/04/2025.
//

#pragma once
#include "json.hpp"
#include "Constants.h"

using json = nlohmann::json;

namespace ObjectMenuDefs {

struct ObjectDef {
    std::string_view definition;
    std::string_view icon;
    bool useIcon = true;
    std::string_view displayName;
    mutable NVGcolor tint;

    bool isEmpty() const { return definition.length() > 0; }
    json getObjectDefinition() const { return json::parse(definition); }
    std::string getDisplayName() const { return std::string(displayName); };
};

struct CategoryBlock
{
    const char* categoryName;
    const ObjectMenuDefs::ObjectDef* items;
    size_t itemCount;
    uint8_t tint[3] = { 0 }; // R G B tint
};

#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

// Control
static constexpr ObjectDef ControlItems[] = {
    { R"({"obj": "Metro"})", ICONS::Metro, true, "Metronome" },
    { R"({"obj": "count"})", ICONS::Count, true, "Counter" },
    { R"({"obj": "IfElse"})", "ifelse", false, "If / Else" },
    { R"({"obj": "select", "outputs": 8 })", "sel", false, "Select" },
    { R"({"obj": "changed"})", "chg", false, "Changed" },
    { R"({"obj": "tag"})", "tag", false, "Tag" },
    { R"({"obj": "filtertag"})", "filtag", false, "Filter by Tag" },
    { R"({"obj": "random"})", "rnd", false, "Random" },
    { R"({"obj": "evdelay", "ms": 100 })", "evdel", false, "Event Delay" },
    { R"({"obj": "loadevent" })", "ldev", false, "Load Event" },
    { R"({"obj": "value" })", "val", false, "Value" },
    { R"({"obj": "mtof"})", "mtof", false, "Note number to Frequency" },
    { R"({"obj": "activemidinotes" })", "", false, "Active MIDI Notes" },
    { R"({"obj": "polynoteout" })", "", false, "Poly Voice Manager" },
};

// UI
static constexpr ObjectDef UIItems[] = {
    { R"({"obj": "ping", "width": 60, "height": 60})", "Png", false, "Ping" },
    { R"({"obj": "dial", "min": 0, "max": 10, "value": 3})", ICONS::Dial, true, "Dial" },
    { R"({"obj": "keyboard" })", "", false, "Piano Keyboard" },
    { R"({"obj": "radiobox"})", "rb", false, "Radio Box" },
    { R"({"obj": "floatbox"})", "fb", false, "Float Box" },
    { R"({"obj": "listbox"})", "lb", false, "List Box" },
    { R"({"obj": "comment"})", "com", false, "Comment" },
    { R"({"obj": "scope"})", "scope", false, "Oscilloscope" },
    { R"({"obj": "spec"})", "spec", false, "Spectrum Analyzer" },
};

// IO
static constexpr ObjectDef IOItems[] = {
    { R"({"obj": "ain"})", "ain", false, "Audio Input" },
    { R"({"obj": "aout"})", ICONS::Aout, true, "Audio Output" },
    { R"({"obj": "notein"})", "MIDIIN", false, "MIDI Note Input" },
};

// Oscillator
static constexpr ObjectDef OscillatorItems[] = {
    { R"({"obj": "Osc", "waveform": "sine", "freq": 440})", ICONS::Osc, true, "Oscillator" },
    { R"({"obj": "tableosc"})", "tosc", false, "Table Oscillator" },
    { R"({"obj": "lfo"})", ICONS::Lfo, true, "LFO" },
    { R"({"obj": "env", "attack": 50, "decay": 50})", ICONS::Adsr, true, "Envelope" },
};

// Effect
static constexpr ObjectDef EffectItems[] = {
    { R"({"obj": "gain"})", "gain", false, "Gain" },
    { R"({"obj": "fdn"})", "fdn", false, "FDN Reverb" },
    { R"({"obj": "drive", "mode": 0 })", "drive", false, "Drive" },
    { R"({"obj": "multitapdelay" })", "mtdel", false, "Multitap Delay" },
    { R"({"obj": "limiter" })", "limit", false, "Limiter" },
    { R"({"obj": "chorus" })", "", false, "Chorus" },
};

// Spectral
static constexpr ObjectDef SpectralItems[] = {
    { R"({"obj": "specfft" })", "fft", false, "FFT" },
    { R"({"obj": "specifft" })", "ifft", false, "Inverse FFT" },
    { R"({"obj": "specmerge" })", "smerge", false, "Spectrum Merge" },
    { R"({"obj": "pitchdetect" })", "pdetect", false, "Pitch Detect" },
};

// Wavetable
static constexpr ObjectDef WavetableItems[] = {
    { R"({"obj": "table", "size": 256 })", "table", false, "Table" },
    { R"({"obj": "tablexfade" })", "txf", false, "Table X Fade" },
    { R"({"obj": "tablexphase" })", "txp", false, "Table X Phase" },
    { R"({"obj": "tablexspectral" })", "txs", false, "Table X Spectral" },
};

// Maths
static constexpr ObjectDef MathsItems[] = {
    { R"({"obj": "Add"})", "", false, "Add" },
    { R"({"obj": "mul"})", "", false, "Multiply" },
    { R"({"obj": "div"})", "", false, "Divide" },
    { R"({"obj": "get"})", "", false, "Get Value" },
    { R"({"obj": "pack", "values": 5 })", "", false, "Pack" },
    { R"({"obj": "strip"})", "", false, "Strip" },
    { R"({"obj": "zerox" })", "", false, "Zero Crossings" },
};

static constexpr ObjectDef MathsUnaryItems[] = {
    { R"({"obj": "intify", "mode": 0 })", "", false, "Intify" },
    { R"({"obj": "abs"})", "", false, "Abs" },
    { R"({"obj": "sin"})", "", false, "Sine" },
    { R"({"obj": "cos"})", "", false, "Cosine" },
    { R"({"obj": "tan"})", "", false, "Tangent" },
    { R"({"obj": "asin"})", "", false, "Arcsin" },
    { R"({"obj": "acos"})", "", false, "Arccos" },
    { R"({"obj": "atan"})", "", false, "Arctan" },
    { R"({"obj": "sqrt"})", "", false, "Sqrt" },
    { R"({"obj": "exp"})", "", false, "Exp" },
    { R"({"obj": "log"})", "", false, "Log" },
    { R"({"obj": "log10"})", "", false, "Log10" },
    { R"({"obj": "sign"})", "", false, "Sign" },
};

static constexpr CategoryBlock objectMenu[] = {
    {"Control", ControlItems, COUNT_OF(ControlItems), {220, 200, 60}},
    {"UI", UIItems, COUNT_OF(UIItems), {90, 160, 200}},
    {"IO", IOItems, COUNT_OF(IOItems), {220, 100, 100}},
    {"Maths", MathsItems, COUNT_OF(MathsItems), {240, 150, 50}},
    {"Maths Unary", MathsUnaryItems, COUNT_OF(MathsUnaryItems), {240, 150, 50}},
    {"Oscillator", OscillatorItems, COUNT_OF(OscillatorItems), {80, 220, 220}},
    {"Effect", EffectItems, COUNT_OF(EffectItems), {200, 100, 220}},
    {"Spectral", SpectralItems, COUNT_OF(SpectralItems), {100, 200, 160}},
    {"Wavetable", WavetableItems, COUNT_OF(WavetableItems), {80, 160, 100}},
};
} // end namespace

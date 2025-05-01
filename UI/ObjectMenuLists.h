//
// Created by alexw on 8/04/2025.
//

#pragma once
#include "json.hpp"
#include "Constants.h"

using json = nlohmann::json;

namespace ObjectMenuDefs
{
    struct ObjectDef
    {
        std::string_view definition;
        std::string_view icon;
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
        uint8_t tint[3] = {0}; // R G B tint
    };

#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

    // Control
    static constexpr ObjectDef ControlItems[] = {
        {R"({"obj": "Metro"})", ICONS::Metro, "Metronome"},
        {R"({"obj": "count"})", ICONS::Count, "Counter"},
        {R"({"obj": "tag"})", "", "Tag"},
        {R"({"obj": "filtertag"})", "", "Filter by Tag"},
        {R"({"obj": "random"})", "", "Random"},
        {R"({"obj": "evdelay", "ms": 100 })", "", "Event Delay"},
        {R"({"obj": "loadevent" })", "", "Load Event"},
        {R"({"obj": "value" })", "", "Value"},
        {R"({"obj": "mtof"})", "", "Note number to Frequency"},
        {R"({"obj": "activemidinotes" })", "", "Active MIDI Notes"},
        {R"({"obj": "polynoteout" })", "", "Poly Voice Manager"},
        {R"({"obj": "get"})", "", "Get Value"},
        {R"({"obj": "pack", "values": 5 })", "", "Pack"},
        {R"({"obj": "unpack", "values": 5 })", "", "UnPack"},
        {R"({"obj": "strip"})", "", "Strip"},
        {R"({"obj": "zerox" })", "", "Zero Crossings"},
        {R"({"obj": "subpatch" })", "", "Subpatch"},
        {R"({"obj": "inlet" })", "", "Inlet"},
        {R"({"obj": "outlet" })", "", "Outlet"},
        {R"({"obj": "d.inlet" })", "", "Data Inlet"},
        {R"({"obj": "d.outlet" })", "", "Data Outlet"},
    };

    // UI
    static constexpr ObjectDef UIItems[] = {
        {R"({"obj": "ping", "width": 60, "height": 60})", "", "Ping"},
        {R"({"obj": "dial", "min": 0, "max": 10, "value": 3})", ICONS::Dial, "Dial"},
        {R"({"obj": "slider" })", "", "Slider"},
        {R"({"obj": "keyboard" })", "", "Piano Keyboard"},
        {R"({"obj": "radiobox"})", "", "Radio Box"},
        {R"({"obj": "floatbox"})", "", "Float Box"},
        {R"({"obj": "listbox"})", "", "List Box"},
        {R"({"obj": "comment"})", "", "Comment"},
        {R"({"obj": "scope"})", "", "Oscilloscope"},
        {R"({"obj": "spec"})", "", "Spectrum Analyzer"},
        {R"({"obj": "pad"})", "", "XY Pad"},
        {R"({"obj": "table", "size": 256 })", "", "Table"},
    };

    // Logic
    static constexpr ObjectDef LogicItems[] = {
        {R"({"obj": "if"})", ICONS::Logic, "If"},
        {R"({"obj": "IfElse"})", ICONS::Logic, "If / Else"},
        {R"({"obj": "select", "outputs": 8 })", ICONS::Logic, "Select"},
        {R"({"obj": "changed"})", ICONS::Logic, "Changed"},
    };

    // IO
    static constexpr ObjectDef IOItems[] = {
        {R"({"obj": "ain"})", ICONS::Mic, "Audio Input"},
        {R"({"obj": "aout"})", ICONS::Aout, "Audio Output"},
        {R"({"obj": "notein"})", "", "MIDI Note Input"},
    };

    // Oscillator
    static constexpr ObjectDef OscillatorItems[] = {
        {R"({"obj": "Osc", "waveform": "sine", "freq": 440})", ICONS::Osc, "Oscillator"},
        {R"({"obj": "tableosc"})", "", "Table Oscillator"},
        {R"({"obj": "lfo"})", ICONS::Lfo, "LFO"},
        {R"({"obj": "env", "attack": 50, "decay": 50})", ICONS::Adsr, "Envelope"},
    };

    // Effect
    static constexpr ObjectDef EffectItems[] = {
        {R"({"obj": "gain"})", "", "Gain"},
        {R"({"obj": "fdn"})", "", "FDN Reverb"},
        {R"({"obj": "drive", "mode": 0 })", "", "Drive"},
        {R"({"obj": "multitapdelay" })", "", "Multitap Delay"},
        {R"({"obj": "limiter" })", "", "Limiter"},
        {R"({"obj": "chorus" })", "", "Chorus"},
        {R"({"obj": "dcblock" })", "", "DC Blocker"},
        {R"({"obj": "bpf" })", "", "Bandpass filter"},
        {R"({"obj": "feedback" })", "", "Feedback"},
        {R"({"obj": "delay", "maxDelayMs": 1000 })", "", "Delay 1sec"},
    };

    // Spectral
    static constexpr ObjectDef SpectralItems[] = {
        {R"({"obj": "specfft" })", "", "FFT"},
        {R"({"obj": "specifft" })", "", "Inverse FFT"},
        {R"({"obj": "specmerge" })", "", "Spectrum Merge"},
        {R"({"obj": "pitchdetect" })", "", "Pitch Detect"},
    };

    // Wavetable
    static constexpr ObjectDef WavetableItems[] = {
        {R"({"obj": "tablexfade" })", "", "Table X Fade"},
        {R"({"obj": "tablexphase" })", "", "Table X Phase"},
        {R"({"obj": "tablexspectral" })", "", "Table X Spectral"},
    };

    // Audio Maths
    static constexpr ObjectDef audioMathsItems[] = {
        {R"({"obj": "audio_add"})",    ICONS::MathBinary, "Add"},
        {R"({"obj": "audio_sub"})",    ICONS::MathBinary, "Subtract"},
        {R"({"obj": "audio_mul"})",    ICONS::MathBinary, "Multiply"},
        {R"({"obj": "audio_div"})",    ICONS::MathBinary, "Divide"},
        {R"({"obj": "audio_min"})",    ICONS::MathBinary, "Min"},
        {R"({"obj": "audio_max"})",    ICONS::MathBinary, "Max"},
        {R"({"obj": "audio_pow"})",    ICONS::MathBinary, "Power"},
        {R"({"obj": "audio_atan2"})",  ICONS::MathBinary, "Atan2"},
        {R"({"obj": "audio_hypot"})",  ICONS::MathBinary, "Hypot"},
    };

    // Maths
    static constexpr ObjectDef MathsItems[] = {
        {R"({"obj": "Add"})", ICONS::MathBinary, "Add"},
        {R"({"obj": "Subtract"})", ICONS::MathBinary, "Subtract"},
        {R"({"obj": "mul"})", ICONS::MathBinary, "Multiply"},
        {R"({"obj": "div"})", ICONS::MathBinary, "Divide"},
    };

    static constexpr ObjectDef MathsUnaryItems[] = {
        {R"({"obj": "intify", "mode": 0 })", ICONS::MathUnary, "Intify"},
        {R"({"obj": "abs"})", ICONS::MathUnary, "Abs"},
        {R"({"obj": "sin"})", ICONS::MathUnary, "Sine"},
        {R"({"obj": "cos"})", ICONS::MathUnary, "Cosine"},
        {R"({"obj": "tan"})", ICONS::MathUnary, "Tangent"},
        {R"({"obj": "asin"})", ICONS::MathUnary, "Arcsin"},
        {R"({"obj": "acos"})", ICONS::MathUnary, "Arccos"},
        {R"({"obj": "atan"})", ICONS::MathUnary, "Arctan"},
        {R"({"obj": "sqrt"})", ICONS::MathUnary, "Sqrt"},
        {R"({"obj": "exp"})", ICONS::MathUnary, "Exp"},
        {R"({"obj": "log"})", ICONS::MathUnary, "Log"},
        {R"({"obj": "log10"})", ICONS::MathUnary, "Log10"},
        {R"({"obj": "sign"})", ICONS::MathUnary, "Sign"},
    };

    static constexpr CategoryBlock objectMenu[] = {
        {"UI", UIItems, COUNT_OF(UIItems), {90, 160, 200}},
        {"Control", ControlItems, COUNT_OF(ControlItems), {220, 200, 60}},
        {"IO", IOItems, COUNT_OF(IOItems), {220, 100, 100}},
        {"Logic", LogicItems, COUNT_OF(LogicItems), {120, 160, 220}},
        {"Audio Maths", audioMathsItems, COUNT_OF(audioMathsItems), {180, 80, 200}},
        {"Maths", MathsItems, COUNT_OF(MathsItems), {240, 150, 50}},
        {"Maths Unary", MathsUnaryItems, COUNT_OF(MathsUnaryItems), {240, 150, 50}},
        {"Oscillator", OscillatorItems, COUNT_OF(OscillatorItems), {80, 220, 220}},
        {"Effect", EffectItems, COUNT_OF(EffectItems), {200, 100, 220}},
        {"Spectral", SpectralItems, COUNT_OF(SpectralItems), {100, 200, 160}},
        {"Wavetable", WavetableItems, COUNT_OF(WavetableItems), {80, 160, 100}},
    };
} // end namespace

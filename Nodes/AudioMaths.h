/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <cmath>

#define DEFINE_BINARY_AUDIO_NODE(CLASS, NAME, FUNC) \
class CLASS : public AudioNode { \
DEFINE_AND_REGISTER_NODE(NAME, NAME, true); \
DEFINE_NODE_ALIASES(NAME); \
public: \
explicit CLASS(std::shared_ptr<NodeContext> context, const json& objParams) \
: AudioNode(context, AudioPort::PortType::Signal, objParams) \
{ \
addInputPort("A", AudioPort::PortType::Signal); \
addInputPort("B", AudioPort::PortType::Signal); \
} \
void processMaths(const float* a, const float* b, float* out, unsigned long frameCount, std::vector<MidiMessage>&) \
{ \
for (unsigned long i = 0; i < frameCount; ++i) \
out[i] = FUNC(a[i], b[i]); \
} \
void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midi) override \
{ \
processMaths(inputPortBuffers[0]->getAudioBuffer(), inputPortBuffers[1]->getAudioBuffer(), outputPortBuffers[0]->getAudioBuffer(), frameCount, midi); \
} \
}; \
REGISTER(CLASS);

DEFINE_BINARY_AUDIO_NODE(AudioAdd,      "audio_add",      [](float a, float b) { return a + b; })
DEFINE_BINARY_AUDIO_NODE(AudioSub,      "audio_sub",      [](float a, float b) { return a - b; })
DEFINE_BINARY_AUDIO_NODE(AudioMul,      "audio_mul",      [](float a, float b) { return a * b; })
DEFINE_BINARY_AUDIO_NODE(AudioDiv,      "audio_div",      [](float a, float b) { return b != 0.0f ? a / b : 0.0f; })
DEFINE_BINARY_AUDIO_NODE(AudioMin,      "audio_min",      [](float a, float b) { return std::fminf(a, b); })
DEFINE_BINARY_AUDIO_NODE(AudioMax,      "audio_max",      [](float a, float b) { return std::fmaxf(a, b); })
DEFINE_BINARY_AUDIO_NODE(AudioPow,      "audio_pow",      [](float a, float b) { return std::powf(a, b); })
DEFINE_BINARY_AUDIO_NODE(AudioAtan2,    "audio_atan2",    [](float a, float b) { return std::atan2f(a, b); })
DEFINE_BINARY_AUDIO_NODE(AudioHypot,    "audio_hypot",    [](float a, float b) { return std::hypotf(a, b); })

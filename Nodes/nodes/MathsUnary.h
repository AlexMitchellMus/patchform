/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../AudioNodeBase.h"
#include <cmath>

#define DEFINE_UNARY_NODE(CLASS, NAME, FUNC) \
class CLASS : public AudioNode { \
DEFINE_AND_REGISTER_NODE(NAME, NAME, false); \
DEFINE_NODE_ALIASES(NAME); \
public: \
explicit CLASS(std::shared_ptr<NodeContext> context, const json& objParams) \
: AudioNode(context, AudioPort::PortType::Data, objParams) \
{ \
addInputPort("In", AudioPort::PortType::Data); \
} \
void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override \
{ \
const auto& events = inputPortBuffers[0]->getEvents(); \
for (auto event : events) {\
if (event->data && event->data->type == DataAtom::DataType::Float) { \
if (Event* e = context->eventPool.getFreeEvent()) \
{ \
e->setTimeStamp(event->getTimeStamp()); \
context->eventPool.addDataAtomTo(e, FUNC(event->getAtomValue(0))); \
addEvent(0, e); \
} \
} \
} \
} \
}; \
REGISTER(CLASS);

// Unary math nodes
DEFINE_UNARY_NODE(Abs,   "abs",    std::fabsf)
DEFINE_UNARY_NODE(Sin,   "sin",    std::sinf)
DEFINE_UNARY_NODE(Cos,   "cos",    std::cosf)
DEFINE_UNARY_NODE(Tan,   "tan",    std::tanf)
DEFINE_UNARY_NODE(Asin,  "asin",   std::asinf)
DEFINE_UNARY_NODE(Acos,  "acos",   std::acosf)
DEFINE_UNARY_NODE(Atan,  "atan",   std::atanf)
DEFINE_UNARY_NODE(Sqrt,  "sqrt",   std::sqrtf)
DEFINE_UNARY_NODE(Exp,   "exp",    std::expf)
DEFINE_UNARY_NODE(Log,   "log",    std::logf)
DEFINE_UNARY_NODE(Log10, "log10",  std::log10f)
DEFINE_UNARY_NODE(Sign,  "sign",   [](float x) { return (x > 0) - (x < 0); })

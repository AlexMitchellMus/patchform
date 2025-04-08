/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// Data
#include "Metronome.h"
#include "Print.h"
#include "Count.h"
#include "If.h"
#include "IfElse.h"
#include "MidiToFreq.h"
#include "Changed.h"
#include "Get.h"
#include "Pack.h"
#include "TagEvent.h"
#include "Strip.h"
#include "FilterTag.h"
#include "ActiveMidiNotes.h"
#include "Random.h"
#include "EventDelay.h"
#include "Select.h"
#include "LoadEvent.h"

// Maths
#include "Multiply.h"
#include "Divide.h"
#include "Add.h"
#include "Intify.h"

// Audio
#include "AudioIn.h"
#include "AudioOut.h"
#include "Value.h"
#include "Add_Audio.h"
#include "Envelope.h"
#include "LFO.h"
#include "Gain.h"
#include "BandPassFilter.h"
#include "Spec.h"
#include "ReverbFDN.h"
#include "Drive.h"
#include "MultiTapDelay.h"
#include "Limiter.h"

// Oscillators
#include "Oscillator.h"
#include "TableOsc.h"
#include "TableIndexOsc.h"

// Mixed
#include "Zerox.h"
#include "PitchDetector.h"

// FFT
#include "SpecFFT.h"
#include "SpecIFFT.h"
#include "SpecMerge.h"

// UI
#include "Dial.h"
#include "RadioBox.h"
#include "FloatBox.h"
#include "ListBox.h"
#include "Ping.h"
#include "Scope.h"
#include "Comment.h"
#include "Table.h"

// Wavetable
#include "TableXFade.h"
#include "TableXPhase.h"
#include "TableXSpectral.h"

// MIDI
#include "MidiNoteIn.h"
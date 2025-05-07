/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// Subpatch
#include "SubpatchInletOutlet.h"
#include "Subpatch.h"
//#include "Poly.h"
//#include "Iterator.h"

// Data
#include "nodes/Metronome.h"
#include "nodes/nodes/Print.h"
#include "nodes/Count.h"
#include "nodes/If.h"
#include "nodes/nodes/IfElse.h"
#include "nodes/MidiToFreq.h"
#include "nodes/Changed.h"
#include "nodes/nodes/Get.h"
#include "nodes/nodes/Pack.h"
#include "nodes/UnPack.h"
#include "nodes/TagEvent.h"
#include "nodes/Strip.h"
#include "nodes/FilterTag.h"
#include "nodes/Random.h"
#include "nodes/EventDelay.h"
#include "nodes/Select.h"
#include "nodes/LoadEvent.h"

// Maths
#include "nodes/Multiply.h"
#include "nodes/Divide.h"
#include "nodes/nodes/Add.h"
#include "nodes/Subtract.h"
#include "nodes/Intify.h"
#include "nodes/MathsUnary.h"

// Audio
#include "nodes/AudioIn.h"
#include "nodes/AudioOut.h"
#include "nodes/Value.h"
#include "nodes/AudioMaths.h"
#include "nodes/Envelope.h"
#include "nodes/LFO.h"
#include "nodes/Gain.h"
#include "nodes/BandPassFilter.h"
#include "nodes/Spec.h"
#include "nodes/ReverbFDN.h"
#include "nodes/Drive.h"
#include "nodes/MultiTapDelay.h"
#include "nodes/Limiter.h"
#include "nodes/Chorus.h"
#include "nodes/DCBlocker.h"
#include "nodes/Feedback.h"
#include "nodes/Delay.h"
#include "nodes/AudioMaths.h"

// Oscillators
#include "nodes/Oscillator.h"
#include "nodes/TableOsc.h"
#include "nodes/TableIndexOsc.h"

// Mixed
#include "nodes/Zerox.h"
#include "nodes/PitchDetector.h"

// FFT
#include "nodes/SpecFFT.h"
#include "nodes/SpecIFFT.h"
#include "nodes/SpecMerge.h"

// UI
#include "nodes/Dial.h"
#include "nodes/RadioBox.h"
#include "nodes/FloatBox.h"
#include "nodes/ListBox.h"
#include "nodes/Ping.h"
#include "nodes/Scope.h"
#include "nodes/Comment.h"
#include "nodes/Table.h"
#include "nodes/Keyboard.h"
#include "nodes/Pad.h"
#include "nodes/Slider.h"

// Wavetable
#include "nodes/TableXFade.h"
#include "nodes/TableXPhase.h"
#include "nodes/TableXSpectral.h"

// MIDI
#include "nodes/MidiNoteIn.h"
#include "nodes/ActiveMidiNotes.h"
#include "nodes/PolyNoteOut.h"

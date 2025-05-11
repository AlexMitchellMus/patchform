/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

// GestureEvent.h or InputEvents.h
#pragma once

enum class GestureKind {
    Pinch,
    Scroll,
    // future: Rotate, Swipe, Tap, etc.
};

struct GestureEvent {
    GestureKind kind;
    float value1;
    float value2;

    static GestureEvent pinch(float delta) {
        return { GestureKind::Pinch, delta, 0.0f };
    }

    static GestureEvent scroll(float dx, float dy) {
        return { GestureKind::Scroll, dx, dy };
    }
};

/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include <vector>
#include <cmath>
#include <optional>
#include <array>
#include <functional>

enum class GestureType {
    None,
    Pinch,
    Scroll
};

struct TouchPoint {
    SDL_FingerID id;
    float x, y;
};

class GestureManager {
public:
    std::function<void(float)> onPinch = [](float) {};
    std::function<void(float, float)> onScroll = [](float, float) {};

    void onTouchDown(const SDL_TouchFingerEvent& tf) {
        if (!gesturesEnabled)
            return;

        activeTouches.push_back({ tf.fingerID, tf.x, tf.y });

        if (activeTouches.size() == 2) {
            // Always recompute sorted order
            std::array<TouchPoint, 2> sorted = { activeTouches[0], activeTouches[1] };
            if (sorted[0].id > sorted[1].id)
                std::swap(sorted[0], sorted[1]);

            const auto& a = sorted[0];
            const auto& b = sorted[1];

            float dx = b.x - a.x;
            float dy = b.y - a.y;

            lastPinchDistance = std::sqrt(dx * dx + dy * dy);
            lastCenter = {
                (a.x + b.x) * 0.5f,
                (a.y + b.y) * 0.5f
            };

            gestureType = GestureType::None;

            // force classification attempt even if no motion has occurred
            update();
        }
    }



    void onTouchUp(const SDL_TouchFingerEvent& tf) {
        if (!gesturesEnabled)
            return;
        std::erase_if(activeTouches, [&](const TouchPoint& t) {
            return t.id == tf.fingerID;
        });
        if (activeTouches.size() < 2) {
            gestureType = GestureType::None;
            reset();
        } else {
            update();
        }
    }

    void onTouchMotion(const SDL_TouchFingerEvent& tf) {
        if (!gesturesEnabled)
            return;
        for (auto& t : activeTouches)
            if (t.id == tf.fingerID) {
                t.x = tf.x;
                t.y = tf.y;
                break;
            }
        update();
    }

    void disableGestures() {
        gesturesEnabled = false;
        cancelGesture(); // stop current gesture
    }

    void enableGestures() {
        gesturesEnabled = true;
    }

    void cancelGesture() {
        gestureType = GestureType::None;
        reset(); // also clears pinch state
    }

    void reset() {
        activeTouches.clear();
        pinchDelta.reset();
        lastPinchDistance = -1.0f;
        lastCenter = {0.0f, 0.0f};
    }

private:
    void update() {
        pinchDelta.reset();

        if (activeTouches.size() != 2)
            return;

        std::array<TouchPoint, 2> sorted = { activeTouches[0], activeTouches[1] };
        if (sorted[0].id > sorted[1].id)
            std::swap(sorted[0], sorted[1]);

        const auto& a = sorted[0];
        const auto& b = sorted[1];

        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        float cx = (a.x + b.x) * 0.5f;
        float cy = (a.y + b.y) * 0.5f;
        std::pair<float, float> center = {cx, cy};

        if (lastPinchDistance > 0.0f) {
            float delta = dist - lastPinchDistance;
            float scrollX = center.first - lastCenter.first;
            float scrollY = center.second - lastCenter.second;

            if (gestureType == GestureType::None) {
                float moveMag = std::sqrt(scrollX * scrollX + scrollY * scrollY);
                float deltaMag = std::abs(delta);

                if (moveMag > 0.001f || deltaMag > 0.001f) {
                    // Prioritize scroll unless pinch is clearly dominant
                    gestureType = (deltaMag > moveMag * 3.0f) ? GestureType::Pinch : GestureType::Scroll;
                } else {
                    return;
                }
            }

            if (gestureType == GestureType::Pinch && std::abs(delta) > 0.001f) {
                pinchDelta = delta * 4.0f;
                onPinch(*pinchDelta);
            } else if (gestureType == GestureType::Scroll &&
                       (std::abs(scrollX) > 0.0005f || std::abs(scrollY) > 0.0005f)) {
                onScroll(scrollX, scrollY);
                       }
        }

        lastPinchDistance = dist;
        lastCenter = center;
    }
    bool gesturesEnabled = true;

    std::vector<TouchPoint> activeTouches;
    std::optional<float> pinchDelta;
    GestureType gestureType = GestureType::None;

    float lastPinchDistance = -1.0f;
    std::pair<float, float> lastCenter = {0.0f, 0.0f};
};

#pragma once

#include <array>
#include <cmath>

namespace SpectralHelpers {

    constexpr float PI = 3.14159265358979323846f;

    constexpr float wrapPi(float x) {
        while (x > PI)  x -= 2 * PI;
        while (x < -PI) x += 2 * PI;
        return x;
    }

    // Constexpr cosine approximation using Taylor series (centered at 0)
    constexpr float constexprCos(float x) {
        x = wrapPi(x);
        float x2 = x * x;
        return 1.0f
            - x2 / 2.0f
            + (x2 * x2) / 24.0f
            - (x2 * x2 * x2) / 720.0f
            + (x2 * x2 * x2 * x2) / 40320.0f;
    }

    template <size_t N>
    constexpr std::array<float, N> makeHannWindow() {
        std::array<float, N> window{};
        for (size_t i = 0; i < N; ++i)
            window[i] = 0.5f * (1.0f - constexprCos(2.0f * PI * i / (N - 1)));
        return window;
    }

    inline void carToPol(float real, float imag, float* length, float* angle) {
        if (length) *length = std::sqrt(real * real + imag * imag);
        if (angle)  *angle  = std::atan2(imag, real);
    }

    inline void polToCar(float length, float angle, float* real, float* imag) {
        if (real)  *real  = length * std::cos(angle);
        if (imag)  *imag  = length * std::sin(angle);
    }

} // namespace SpectralHelpers

#pragma once

class MainVolumeMeter : public pptk::Component {
public:
    std::function<void(float)> onVolumeChange = [](float) {};

    MainVolumeMeter() = default;

    void setValue(float peakL, float peakR, float holdL, float holdR)
    {
        peakL = std::clamp(peakL, 1e-6f, 1.0f);
        peakR = std::clamp(peakR, 1e-6f, 1.0f);
        holdL = std::clamp(holdL, 1e-6f, 1.0f);
        holdR = std::clamp(holdR, 1e-6f, 1.0f);

        constexpr float dbRange = 40.0f;
        float peakDbL = 20.0f * std::log10(peakL);
        float peakDbR = 20.0f * std::log10(peakR);
        float holdDbL = 20.0f * std::log10(holdL);
        float holdDbR = 20.0f * std::log10(holdR);

        float targetLeft = std::clamp((peakDbL + dbRange) / dbRange, 0.0f, 1.0f);
        float targetRight = std::clamp((peakDbR + dbRange) / dbRange, 0.0f, 1.0f);
        holdLeftNorm = std::clamp((holdDbL + dbRange) / dbRange, 0.0f, 1.0f);
        holdRightNorm = std::clamp((holdDbR + dbRange) / dbRange, 0.0f, 1.0f);

        // Envelope-style decay: very fast attack, slow release
        constexpr float attackFactor = 0.99f;  // fast attack
        constexpr float releaseFactor = 0.07f; // slow release
        meterLeftNorm += (targetLeft - meterLeftNorm) * ((targetLeft > meterLeftNorm) ? attackFactor : releaseFactor);
        meterRightNorm += (targetRight - meterRightNorm) * ((targetRight > meterRightNorm) ? attackFactor : releaseFactor);

        float meterWidth = width - (height * 2) * getAccumulatedScale();
        float newPos = (meterLeftNorm + meterRightNorm + holdLeftNorm + holdRightNorm) * 0.25f * meterWidth;

        if (std::abs(newPos - peakMeterPos) > 0.5f) { // half a pixel increments
            peakMeterPos = newPos;
            repaint();
        }
    }

    float getLeftVal() const { return meterLeftNorm; }
    float getRightVal() const { return meterRightNorm; }

    void snapToUnity()
    {
        float meterWidth = width - height;
        volume = unityPosNorm;
        onVolumeChange(volume);
        thumbPixelPos = meterWidth * unityPosNorm;
        repaint();
    }

    void mouseButtonDown(pptk::CompEvent& e) override
    {
        if (e.sdlEvent.button.clicks == 2)
            snapToUnity();
    }

    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override
    {
        float meterWidth = width - height;
        thumbPixelPos = std::clamp(thumbPixelPos + delta.x, 0.0f, meterWidth);

        volume = thumbPixelPos / meterWidth;
        onVolumeChange(volume);
        repaint();
    }

    void resized() override
    {
        if (!initialized && width > 0 && height > 0) {
            snapToUnity();
            initialized = true;
        }
    }

    void render(NVGcontext* nvg, const pptk::Theme& theme) override
    {
        nvgBeginPath(nvg);
        auto bgColor = nvgRGBA(23, 23, 23, 255);
        float halfHeight = getHeight() * 0.5f;

        nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), bgColor, bgColor, halfHeight);

        const float meterWidth = width - (halfHeight * 2);
        const float meterX = halfHeight;
        const float meterH = height * 0.2f;
        const auto meterBgCol = nvgRGBA(40, 40, 40, 255);

        const float leftX = meterWidth * meterLeftNorm;
        const float rightX = meterWidth * meterRightNorm;
        const float holdLeftX = meterWidth * holdLeftNorm;
        const float holdRightX = meterWidth * holdRightNorm;

        // === Left Channel ===
        float meterY = height * 0.25f;
        nvgDrawRoundedRect(nvg, meterX, meterY, meterWidth, meterH, meterBgCol, meterBgCol, 0);
        {
            const uint8_t alphaL = static_cast<uint8_t>(std::clamp(holdLeftNorm * 80.0f, 0.0f, 80.0f));
            const auto holdColL = nvgRGBA(180, 180, 180, alphaL);
            nvgDrawRoundedRect(nvg, meterX + holdLeftX - 1, meterY, 2, meterH, holdColL, holdColL, 0);

            const auto colL = meterLeftNorm > 0.99f ? nvgRGB(255, 0, 0) : nvgRGBA(28, 73, 119, 180);
            nvgDrawRoundedRect(nvg, meterX, meterY, leftX, meterH, colL, colL, 0);
        }

        // === Right Channel ===
        meterY = height * 0.55f;
        nvgDrawRoundedRect(nvg, meterX, meterY, meterWidth, meterH, meterBgCol, meterBgCol, 0);
        {
            const uint8_t alphaR = static_cast<uint8_t>(std::clamp(holdRightNorm * 80.0f, 0.0f, 80.0f));
            const auto holdColR = nvgRGBA(180, 180, 180, alphaR);
            nvgDrawRoundedRect(nvg, meterX + holdRightX - 1, meterY, 2, meterH, holdColR, holdColR, 0);

            const auto colR = meterRightNorm > 0.99f ? nvgRGB(255, 0, 0) : nvgRGBA(28, 73, 119, 180);
            nvgDrawRoundedRect(nvg, meterX, meterY, rightX, meterH, colR, colR, 0);
        }

        // === Thumb ===
        auto thumbCol = nvgRGBA(94, 94, 94, 70);
        auto thumbOutlineCol = thumbCol;
        thumbOutlineCol.a = 130;
        nvgDrawRoundedRect(nvg, thumbPixelPos + 3, 3, height - 6, height - 6, thumbCol, thumbOutlineCol, height * 0.5f);
        nvgDrawRoundedRect(nvg, thumbPixelPos + 2, 2, height - 4, height - 4, nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 50), height * 0.5f);
    }

private:
    float meterLeftNorm = 0.0f;
    float meterRightNorm = 0.0f;
    float holdLeftNorm = 0.0f;
    float holdRightNorm = 0.0f;
    float peakMeterPos = 0;

    float volume = 1.0f;
    float thumbPixelPos = 0.0f;

    const float unityPosNorm = 0.75f;
    bool initialized = false;
};

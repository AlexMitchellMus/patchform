#pragma once

class MainVolumeMeter : public pptk::Component
{
public:
    std::function<void(float)> onVolumeChange = [](float){};

    MainVolumeMeter() = default;

    void setValue(float left, float right)
    {
        left = std::clamp(left, 1e-6f, 1.0f);
        right = std::clamp(right, 1e-6f, 1.0f);

        leftMeterPeakVal = 20.0f * std::log10(left);
        rightMeterPeakVal = 20.0f * std::log10(right);

        constexpr float dbRange = 40.0f;
        float meterLeft = std::clamp((leftMeterPeakVal + dbRange) / dbRange, 0.0f, 1.0f);
        float meterRight = std::clamp((rightMeterPeakVal + dbRange) / dbRange, 0.0f, 1.0f);

        float meterWidth = width - (height * 2) * getAccumulatedScale();
        int newPos = static_cast<int>((meterLeft + meterRight) * 0.5f * meterWidth * 2);

        if (peakMeterPos != newPos)
        {
            peakMeterPos = newPos;
            repaint();
        }

        meterLeftNorm = meterLeft;
        meterRightNorm = meterRight;
    }

    float getLeftVal() { return meterLeftNorm; }
    float getRightVal() { return meterRightNorm; }

    // Snap to unity gain
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
        {
            snapToUnity();
        }
    }

    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override
    {
        float meterWidth = width - height;
        thumbPixelPos += delta.x;
        thumbPixelPos = std::clamp(thumbPixelPos, 0.0f, meterWidth);

        volume = thumbPixelPos / meterWidth;
        onVolumeChange(volume);
        repaint();
    }

    void resized() override
    {
        if (!initialized && width > 0 && height > 0)
        {
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

        float meterWidth = width - (halfHeight * 2);
        float meterX = halfHeight;
        float meterY = height * 0.25f;
        float meterH = height * 0.2f;

        // Left channel
        auto meterBgCol = nvgRGBA(40, 40, 40, 255);
        nvgDrawRoundedRect(nvg, meterX, meterY, meterWidth, meterH, meterBgCol, meterBgCol, 0);
        float leftX = meterWidth * meterLeftNorm;
        auto colL = meterLeftNorm > 0.99f ? nvgRGB(255, 0, 0) : nvgRGBA(28, 73, 119, 180);
        nvgDrawRoundedRect(nvg, meterX, meterY, leftX, meterH, colL, colL, 0);

        // Right channel
        meterY = height * 0.55f;
        nvgDrawRoundedRect(nvg, meterX, meterY, meterWidth, meterH, meterBgCol, meterBgCol, 0);
        float rightX = meterWidth * meterRightNorm;
        auto colR = meterRightNorm > 0.99f ? nvgRGB(255, 0, 0) : nvgRGBA(28, 73, 119, 180);
        nvgDrawRoundedRect(nvg, meterX, meterY, rightX, meterH, colR, colR, 0);

        // Volume thumb control
        auto thumbCol = nvgRGBA(88, 88, 88, 50);
        auto thumbOutlineCol = thumbCol;
        thumbOutlineCol.a = 130;

        nvgDrawRoundedRect(nvg, thumbPixelPos + 3, 3, height - 6, height - 6, thumbCol, thumbOutlineCol, height * 0.5f);
        // Dark circle glow around thumb
        nvgBeginPath(nvg);
        nvgCircle(nvg, thumbPixelPos + (height * 0.5f), height * 0.5f, (height - 2.0f) * 0.5f);  // slightly larger than thumb
        nvgStrokeColor(nvg, nvgRGBA(0, 0, 0, 24));
        nvgStrokeWidth(nvg, 2.0f);
        nvgLineStyle(nvg, NVG_LINE_SOLID);
        nvgStroke(nvg);
    }

private:
    float leftMeterPeakVal = 0.0f;
    float rightMeterPeakVal = 0.0f;
    float meterLeftNorm = 0.0f;
    float meterRightNorm = 0.0f;
    int peakMeterPos = 0;

    float volume;
    float thumbPixelPos = 0.0f;

    const float unityVolume = 1.0f;
    const float unityPosNorm = 0.75f;

    bool initialized = false;
};

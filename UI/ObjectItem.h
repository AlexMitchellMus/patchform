#pragma once


class ObjectItem : public pptk::Component
{
public:

    std::function<void()> onClick;

    ObjectItem(Object* canvasObj) : obj(canvasObj)
    {
        name = obj->getName();
        isSelected = obj->getIsSelected();

        setName("object item: " + name);
    }

    void update()
    {
        isSelected = obj->getIsSelected();
        resetHovered();
    }

    void mouseEnter(pptk::CompEvent& e) override
    {
        isHovered = true;
        repaint();
    }

    void mouseLeave(pptk::CompEvent& e) override
    {
        isHovered = false;
        repaint();
    }

    void mouseButtonDown(pptk::CompEvent& e) override
    {
        onClick();
        repaint();
    }

    void render(NVGcontext* nvg, const pptk::Theme& theme) override
    {
        if (isSelected || isHovered)
        {
            NVGcolor bg = isSelected
                ? nvgRGB(43, 43, 43)
                : nvgRGBA(43, 43, 43, static_cast<unsigned char>(255 * 0.4f));

            nvgDrawRoundedRect(nvg, 8, 4, width - 16, height - 8, bg, bg, 6.0f);
        }

        nvgFillColor(nvg, nvgRGB(220, 220, 220));
        nvgFontFace(nvg, "Regular");
        nvgFontSize(nvg, 14.0f);
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(nvg, 24, ((height - 10) * 0.5f) + 5, name.c_str(), nullptr);
    }

    Object* getObject()
    {
        return obj;
    }

    void resetHovered()
    {
        isHovered = false;
        repaint();
    }
private:
    Object* obj;
    std::string name;
    bool isSelected = false;
    bool isHovered = false;
};
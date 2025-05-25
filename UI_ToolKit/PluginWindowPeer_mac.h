#pragma once

#include "PluginWindowPeer.h"

class PluginWindowPeer_mac : public PluginWindowPeer {
public:
    explicit PluginWindowPeer_mac(void* nativeHandle);

    void swapBuffers() override;
    void setTitle(const std::string&) override;
    void setUserSize(int, int) override;
    void getUserSize(int& w, int& h) const override;
    void getWindowSize(int& w, int& h) const override;
    void getDrawableSize(int& w, int& h) const override;
    void setSize(int, int) override;
    bool isMaximized() const override;
    void setMaximized(bool) override;
    bool getIsProgrammaticResize() override;
};

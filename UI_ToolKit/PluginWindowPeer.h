#pragma once

#include "WindowPeer.h"
#include <memory>

class PluginWindowPeer : public WindowPeer {
public:
    static std::unique_ptr<PluginWindowPeer> create(void* nativeHandle);
    virtual ~PluginWindowPeer() = default;

protected:
    explicit PluginWindowPeer(void* nativeHandle) : handle(nativeHandle) {}
    void* getNativeHandle() const override { return handle; }

    void* handle;
};
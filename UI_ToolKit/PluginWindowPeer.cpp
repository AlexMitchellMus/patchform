#include "PluginWindowPeer.h"

#if defined(__APPLE__)
#include "PluginWindowPeer_mac.h"
#endif

std::unique_ptr<PluginWindowPeer> PluginWindowPeer::create(void* nativeHandle) {
#if defined(__APPLE__)
    return std::make_unique<PluginWindowPeer_mac>(nativeHandle);
#else
    return nullptr;
#endif
}
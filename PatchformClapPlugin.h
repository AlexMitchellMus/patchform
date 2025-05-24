#pragma once

#include "clap/include/clap/clap.h"
#include <memory>

#include "PluginLogger.h"

#ifdef __OBJC__
@class PatchformGLView;
#else
class PatchformGLView;
#endif

class PatchformApp;
class PatchformClapPlugin {
    public:

    ~PatchformClapPlugin();

    PatchformGLView* glView = nullptr;

    static constexpr clap_plugin_descriptor desc = {
        .clap_version = CLAP_VERSION,
        .id = "dev.patchform",
        .name = "Patchform",
        .vendor = "Patchform",
        .url = "https://patchform.dev",
        .version = "0.1",
        .description = "Modular node system",
        .features = (const char*[]) {
            CLAP_PLUGIN_FEATURE_INSTRUMENT,
            CLAP_PLUGIN_FEATURE_SYNTHESIZER,
            CLAP_PLUGIN_FEATURE_STEREO,
            CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
            nullptr,
        },
    };

    clap_plugin pluginStruct {};
    const clap_host* host = nullptr;

    std::unique_ptr<PatchformApp> app;

    const static clap_plugin* create(const clap_host* host);
    static void destroy(const clap_plugin* plugin);
    static bool activate(const clap_plugin* plugin, double sampleRate, uint32_t minFrames, uint32_t maxFrames);
    static void deactivate(const clap_plugin* plugin);
    static clap_process_status process(const clap_plugin* plugin, const clap_process* process);
    static const void* getExtension(const clap_plugin* plugin, const char* id);
    static bool start_processing(const clap_plugin* plugin) { return true; }
    static void stop_processing(const clap_plugin* plugin) {}
    static void reset(const clap_plugin* plugin) {}
    static void on_main_thread(const clap_plugin* plugin) {}

    void setParentView(void* cocoaView);
};

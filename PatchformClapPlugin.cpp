#include "PatchformClapPlugin.h"

static PatchformClapPlugin* instance = nullptr;

bool init(const clap_plugin* plugin) {
    return true;
}

const clap_plugin* PatchformClapPlugin::create(const clap_host* host) {
    instance = new PatchformClapPlugin();
    instance->host = host;

    instance->pluginStruct = {
        .desc = &desc,
        .init = &init,
        .activate = activate,
        .deactivate = deactivate,
        .start_processing = start_processing,
        .stop_processing = stop_processing,
        .reset = reset,
        .process = process,
        .get_extension = getExtension,
        .on_main_thread = on_main_thread,
        .destroy = destroy
    };

    return &instance->pluginStruct; // defer app creation until GUI init
}

void PatchformClapPlugin::destroy(const clap_plugin* plugin) {
    delete instance;
    instance = nullptr;
}

bool PatchformClapPlugin::activate(const clap_plugin* plugin, double sampleRate, uint32_t minFrames, uint32_t maxFrames) {
    // You can store sampleRate/minFrames if needed
    return true;
}

void PatchformClapPlugin::deactivate(const clap_plugin* plugin) {
    // Nothing yet
}

clap_process_status PatchformClapPlugin::process(const clap_plugin* plugin, const clap_process* process) {
    // TODO: implement Patchform audio processing using process->audio_inputs/outputs
    return CLAP_PROCESS_CONTINUE;
}

const void* PatchformClapPlugin::getExtension(const clap_plugin* plugin, const char* id) {
    // TODO: return GUI extension here when implemented
    return nullptr;
}

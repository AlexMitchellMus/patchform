#include "PatchformClapPlugin.h"

#include "PluginLogger.h"
#include "clap/ext/gui.h"
#include "PatchformClapPluginGUI.h"
#include "../UI/PatchformApp.h"
#include "PatchformGLView.h"

bool init(const clap_plugin* plugin)
{
    return true;
}

PatchformClapPlugin::~PatchformClapPlugin() {}

const clap_plugin* PatchformClapPlugin::create(const clap_host* host)
{
    auto* self = new PatchformClapPlugin();
    self->host = host;

    self->pluginStruct = {
        .desc = &desc,
        .init = &init,
        .plugin_data = self,
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

    return &self->pluginStruct;
}

void PatchformClapPlugin::on_main_thread(const clap_plugin* plugin)
{
}

double PatchformClapPlugin::getHighResTime()
{
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return duration<double>(steady_clock::now() - start).count();
}

void PatchformClapPlugin::destroy(const clap_plugin* plugin)
{
    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);
    delete self;
}

bool PatchformClapPlugin::activate(const clap_plugin* plugin, double sampleRate, uint32_t minFrames, uint32_t maxFrames)
{
    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);
    self->app = std::make_unique<PatchformApp>(sampleRate, maxFrames);
    LOG_TO_FILE("plugin activate - SR: " << sampleRate << " max frames: " << maxFrames);
    return true;
}

void PatchformClapPlugin::deactivate(const clap_plugin* plugin)
{
    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);
    self->app.reset();
}

clap_process_status PatchformClapPlugin::process(const clap_plugin* plugin, const clap_process* process)
{
    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);

    const uint32_t numFrames = process->frames_count;

    const float* inL = nullptr;
    const float* inR = nullptr;
    float* outL = nullptr;
    float* outR = nullptr;

    if (process->audio_inputs_count > 0) {
        auto& inBuf = process->audio_inputs[0];
        inL = inBuf.channel_count > 0 ? inBuf.data32[0] : nullptr;
        inR = inBuf.channel_count > 1 ? inBuf.data32[1] : nullptr;
    }

    if (process->audio_outputs_count > 0) {
        auto& outBuf = process->audio_outputs[0];
        outL = outBuf.channel_count > 0 ? outBuf.data32[0] : nullptr;
        outR = outBuf.channel_count > 1 ? outBuf.data32[1] : nullptr;
    }

    std::vector<float> inputLR(numFrames * 2);
    for (uint32_t i = 0; i < numFrames; ++i) {
        inputLR[i] = inL ? inL[i] : 0.0f;             // Left
        inputLR[i + numFrames] = inR ? inR[i] : 0.0f; // Right
    }

    std::vector<float> outputLR(numFrames * 2);
    self->app->pluginProcess(inputLR.data(), outputLR.data(), numFrames);

    // Unpack back into CLAP non-interleaved
    for (uint32_t i = 0; i < numFrames; ++i) {
        if (outL) outL[i] = outputLR[i];
        if (outR) outR[i] = outputLR[i + numFrames];
    }

    return CLAP_PROCESS_CONTINUE;
}

static uint32_t audio_port_count(const clap_plugin*, bool is_input) {
    return 1;
}

static bool audio_port_info(const clap_plugin*, uint32_t index, bool is_input, clap_audio_port_info* info) {
    if (index != 0) return false;

    info->id = is_input ? 0 : 1;
    info->name[0] = '\0';
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;

    return true;
}

static const clap_plugin_audio_ports audioPorts = {
    .count = audio_port_count,
    .get = audio_port_info
};

const void* PatchformClapPlugin::getExtension(const clap_plugin* plugin, const char* id)
{
    if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
        return &audioPorts;

    if (strcmp(id, CLAP_EXT_GUI) == 0)
        return &guiExt;

    return nullptr;
}

void PatchformClapPlugin::setParentView(void* cocoaView)
{
}

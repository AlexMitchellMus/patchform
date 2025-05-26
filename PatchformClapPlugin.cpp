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
    self->app = std::make_unique<PatchformApp>(sampleRate, 64);
    return true;
}

void PatchformClapPlugin::deactivate(const clap_plugin* plugin)
{
    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);
    self->app.reset();
}

clap_process_status PatchformClapPlugin::process(const clap_plugin* plugin, const clap_process*)
{
    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);
    return CLAP_PROCESS_CONTINUE;
}

const void* PatchformClapPlugin::getExtension(const clap_plugin* plugin, const char* id)
{
    if (strcmp(id, CLAP_EXT_GUI) == 0) {
        return &guiExt;
    }
    return nullptr;
}

void PatchformClapPlugin::setParentView(void* cocoaView)
{
}

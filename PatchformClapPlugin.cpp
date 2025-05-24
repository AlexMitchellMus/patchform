#include "PatchformClapPlugin.h"

#include "PluginLogger.h"
#include "clap/ext/gui.h"
#include "PatchformClapPluginGUI.h"
#include "../UI/PatchformApp.h"

static PatchformClapPlugin* instance = nullptr;

bool init(const clap_plugin* plugin)
{
    return true;
}

PatchformClapPlugin::~PatchformClapPlugin()
{

}

const clap_plugin* PatchformClapPlugin::create(const clap_host* host)
{
    instance = new PatchformClapPlugin();
    instance->host = host;
    instance->app = std::make_unique<PatchformApp>(44100, 64);

    instance->pluginStruct = {
        .desc = &desc,
        .init = &init,
        .plugin_data = instance,
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

    return &instance->pluginStruct;
}

void PatchformClapPlugin::destroy(const clap_plugin* plugin)
{
    delete instance;
    instance = nullptr;
}

bool PatchformClapPlugin::activate(const clap_plugin* plugin, double, uint32_t, uint32_t)
{
    return true;
}

void PatchformClapPlugin::deactivate(const clap_plugin* plugin) {}

clap_process_status PatchformClapPlugin::process(const clap_plugin*, const clap_process*)
{
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
    if (app->initializePluginGUI(cocoaView, CLAP_WINDOW_API_COCOA))
        logToFile("successfully initialized plugin GUI");
    else
        logToFile("failed to initialize plugin GUI");
}

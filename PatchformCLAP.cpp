// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#include "clap/include/clap/clap.h"
#include "clap/include/clap/ext/gui.h"
#include "clap/ext/log.h"
#include "PatchformClapPlugin.h"

#include <cstring>

extern "C" {

static const clap_plugin *create_plugin(const clap_plugin_factory *, const clap_host *host, const char *plugin_id) {
    const clap_host_log *host_log = nullptr;

    if (host && host->get_extension)
        host_log = (const clap_host_log *) host->get_extension(host, CLAP_EXT_LOG);

    if (host_log && host_log->log) {
        std::string msg = "Patchform:: create_plugin() called with ID: " + std::string(plugin_id);
        host_log->log(host, CLAP_LOG_DEBUG, msg.c_str());
    }

    if (strcmp(plugin_id, PatchformClapPlugin::desc.id) != 0)
        return nullptr;

    return PatchformClapPlugin::create(host);
}

static std::mutex entry_init_guard;
static int entry_init_counter = 0;
static std::string pluginPath;

static bool clap_init(const char *plugin_path)
{
    pluginPath = plugin_path;
    return true;
}

// ---- Entry Hooks ----
static bool init(const char *plugin_path)
{
    std::lock_guard<std::mutex> guard(entry_init_guard);
    const int cnt = ++entry_init_counter;
    if (cnt > 1)
        return true;
    if (clap_init(plugin_path))
        return true;

    entry_init_counter = 0;
    return true;
}

static void deinit()
{
    std::lock_guard<std::mutex> guard(entry_init_guard);
    pluginPath.clear();
}

// ---- Factory Functions ----
static uint32_t get_plugin_count(const clap_plugin_factory *)
{
    return 1;
}

static const clap_plugin_descriptor *get_plugin_descriptor(const clap_plugin_factory *, uint32_t index)
{
    return (index == 0) ? &PatchformClapPlugin::desc : nullptr;
}

// ---- Plugin Factory ----
static const clap_plugin_factory pluginFactory = {
    .get_plugin_count = get_plugin_count,
    .get_plugin_descriptor = get_plugin_descriptor,
    .create_plugin = create_plugin
};

static const void *get_factory_cb(const char *factory_id) {
    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
        return &pluginFactory;
    return nullptr;
}

}// extern "C"

// ---- CLAP Entry Point ----

extern "C" CLAP_EXPORT
const clap_plugin_entry clap_entry = {
    .clap_version = CLAP_VERSION,
    .init = init,
    .deinit = deinit,
    .get_factory = get_factory_cb
};
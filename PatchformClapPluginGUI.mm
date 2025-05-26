#include "UI/PatchformApp.h"
#import <AppKit/AppKit.h>
#include "clap/clap.h"
#include "clap/ext/gui.h"
#include "PatchformClapPluginGUI.h"
#include "PatchformClapPlugin.h"
#include "PluginLogger.h"
#import "PatchformGLViewInternal.h"
#include <sstream>

static NSWindow* parentWindow = nil;

#define CLAP_EXPORT __attribute__((used)) __attribute__((visibility("default")))

extern "C" {

CLAP_EXPORT bool gui_is_api_supported(const clap_plugin*, const char* api, bool is_floating)
{
    LOG_TO_FILE("gui_is_api_supported: " + std::string(api));
    return strcmp(api, CLAP_WINDOW_API_COCOA) == 0;
}

CLAP_EXPORT bool gui_get_preferred_api(const clap_plugin*, const char** api, bool* is_floating)
{
    LOG_TO_FILE("gui_get_preferred_api");
    *api = CLAP_WINDOW_API_COCOA;
    *is_floating = false;
    return true;
}

CLAP_EXPORT bool gui_create(const clap_plugin* plugin, const char*, bool)
{
    LOG_TO_FILE("gui_create");
    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);

    if (!self->glView) {
        // Create with dummy rect for now; real size is set in set_parent
        NSRect dummy = NSMakeRect(0, 0, 10, 10);
        self->glView = [[PatchformGLView alloc] initWithFrame:dummy app:self->app.get()];
        [self->glView setWantsBestResolutionOpenGLSurface:YES];
        [self->glView setNeedsDisplay:YES];
    }

    return self->glView != nil;
}

CLAP_EXPORT bool gui_set_parent(const clap_plugin* plugin, const clap_window* window)
{
    LOG_TO_FILE("gui_set_parent");

    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);

    if (!window || strcmp(window->api, CLAP_WINDOW_API_COCOA) != 0 || !window->cocoa) {
        LOG_TO_FILE("gui_set_parent no window!");
        return false;
    }

    if (!self->glView) {
        LOG_TO_FILE("gui_set_parent: glView is null!");
        return false;
    }

    NSView* parent = (__bridge NSView*)window->cocoa;
    NSRect frame = [parent bounds];

    [self->glView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self->glView setFrame:frame];
    [parent addSubview:self->glView];
    [self->glView setNeedsDisplay:YES];
    [self->glView startDisplayLinkIfNeeded];

    Dl_info info;
    std::filesystem::path bundlePath;

    if (dladdr((void*)gui_set_parent, &info)) {
        std::filesystem::path path(info.dli_fname);
        for (int i = 0; i < 10 && !path.empty(); ++i) {
            if (path.filename() == "Contents") {
                bundlePath = path / "Resources";
                break;
            }
            path = path.parent_path();
        }
    }

    self->app->initializePluginGUI(parent, CLAP_WINDOW_API_COCOA, bundlePath);

    LOG_TO_FILE("gui_set_parent: added glView to parent");

    return true;
}


CLAP_EXPORT void gui_destroy(const clap_plugin* plugin)
{
    LOG_TO_FILE("gui_destroy");
    auto* self = static_cast<PatchformClapPlugin*>(plugin->plugin_data);

    self->app->destroyPluginGUI();

    if (self->glView) {
        [self->glView removeFromSuperview];
        self->glView = nil;
    }
}

CLAP_EXPORT bool gui_show(const clap_plugin* plugin)
{
      LOG_TO_FILE("gui_show");
      return true;
}

CLAP_EXPORT bool gui_hide(const clap_plugin*)
{
    LOG_TO_FILE("gui_hide");
    return true;
}

CLAP_EXPORT bool gui_set_scale(const clap_plugin*, double scale) {
    LOG_TO_FILE("gui_set_scale");
    return true;
}

CLAP_EXPORT bool gui_get_size(const clap_plugin*, uint32_t* width, uint32_t* height) {
    *width = 800;
    *height = 600;
    LOG_TO_FILE("gui_get_size");
    return true;
}

CLAP_EXPORT bool gui_can_resize(const clap_plugin*) {
    LOG_TO_FILE("gui_can_resize");
    return false;
}

CLAP_EXPORT bool gui_get_resize_hints(const clap_plugin*, clap_gui_resize_hints* hints) {
    LOG_TO_FILE("gui_get_resize_hints");
    return false;
}

CLAP_EXPORT bool gui_adjust_size(const clap_plugin*, uint32_t* width, uint32_t* height) {
    LOG_TO_FILE("gui_adjust_size");
    return true;
}

CLAP_EXPORT bool gui_set_size(const clap_plugin*, uint32_t width, uint32_t height) {
    LOG_TO_FILE("gui_set_size");
    return true;
}

CLAP_EXPORT bool gui_set_transient(const clap_plugin*, const clap_window* window) {
    LOG_TO_FILE("gui_set_transient");
    return true;
}

CLAP_EXPORT void gui_suggest_title(const clap_plugin*, const char* title) {
    LOG_TO_FILE(std::string("gui_suggest_title: ") + title);
}

}

extern "C" __attribute__((visibility("default")))
const clap_plugin_gui guiExt  = {
    .is_api_supported = gui_is_api_supported,
    .get_preferred_api = gui_get_preferred_api,
    .create = gui_create,
    .destroy = gui_destroy,
    .set_scale = gui_set_scale,
    .get_size = gui_get_size,
    .can_resize = gui_can_resize,
    .get_resize_hints = gui_get_resize_hints,
    .adjust_size = gui_adjust_size,
    .set_size = gui_set_size,
    .set_parent = gui_set_parent,
    .set_transient = gui_set_transient,
    .suggest_title = gui_suggest_title,
    .show = gui_show,
    .hide = gui_hide,
};


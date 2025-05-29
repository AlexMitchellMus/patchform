#include "SDL3/SDL_main.h"
#include "UI/PatchformApp.h"
#include "PatchformEnvironment.h"

extern "C" SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    PatchformEnvironment::setStandalone();

    // Make sure that we are populating isStandalone() correctly!
    assert(PatchformEnvironment::isStandalone() == true && "Patchform build mode should be standalone!");

    auto* app = new PatchformApp(44100, 64);
    app->loadDefaultPatch();
    if (!app->initialize()) return SDL_APP_FAILURE;
    *appstate = app;
    return SDL_APP_CONTINUE;
}

extern "C" SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    auto* app = static_cast<PatchformApp*>(appstate);
    app->pendingEvents.enqueue(*event);
    return SDL_APP_CONTINUE;
}

extern "C" SDL_AppResult SDL_AppIterate(void* appstate) {
    auto* app = static_cast<PatchformApp*>(appstate);
    return app->nextFrame() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

extern "C" void SDL_AppQuit(void* appstate, SDL_AppResult) {
    auto* app = static_cast<PatchformApp*>(appstate);
    app->shutdown();
    // FIXME: Deleting the app causes a segfault??
    //delete app;
}


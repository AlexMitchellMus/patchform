#include <iostream>
#include "SDL3/SDL_main.h"
#include "UI/PatchformApp.h"


int main(int argc, char* argv[]) {
    unsigned long frameCount = 64;
    int sampleRate = 44100;

    PatchformApp app(sampleRate, frameCount);

#ifdef PATCHFORM_GIT_VERSION && PATCHFORM_GIT_HASH
    std::cout << "Patchform version: " << PATCHFORM_GIT_VERSION << " hash: " << PATCHFORM_GIT_HASH << std::endl;
#endif

    if (!app.initialize()) {
        std::cerr << "Failed to initialize PatchformApp" << std::endl;
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}

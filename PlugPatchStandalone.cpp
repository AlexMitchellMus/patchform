#include <iostream>
#include "SDL3/SDL_main.h"
#include "UI/PatchformApp.h"

#include "GitInfo.h"

#define PATCHFORM_STANDALONE

int main(int argc, char* argv[]) {
    unsigned long frameCount = 64;
    int sampleRate = 44100;

    PatchformApp app(sampleRate, frameCount);

    std::cout << "Patchform version: " << patchform_git_version  << " hash: " << patchform_git_hash << std::endl;

    if (!app.initialize()) {
        std::cerr << "Failed to initialize PatchformApp" << std::endl;
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}

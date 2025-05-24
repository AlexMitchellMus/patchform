#include "PatchformBuildMode.h"

#if defined(PATCHFORM_PLUGIN)
PatchformBuildMode patchformBuildMode(PatchformBuildMode::Mode::Plugin);
#else
PatchformBuildMode patchformBuildMode(PatchformBuildMode::Mode::Standalone);
#endif
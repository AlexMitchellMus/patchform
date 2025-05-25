#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    // Call this from C++ to make the OpenGL context current on the given NSView*
    void makeGLViewCurrent(void* view);

#ifdef __cplusplus
}
#endif
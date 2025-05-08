/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include "nanovg.h"

#if defined(_WIN32)
  #define SDK_EXPORT __declspec(dllexport)
#else
  #define SDK_EXPORT __attribute__((visibility("default")))
#endif

struct NVGcontext;
struct NVGLUframebuffer;

#ifdef __cplusplus
extern "C" {
#endif

    SDK_EXPORT NVGcontext* createNanoVGContext(int flags);
    SDK_EXPORT void destroyNanoVGContext(NVGcontext* ctx);
    SDK_EXPORT void nanoVGBlitFramebuffer(NVGcontext* ctx, NVGLUframebuffer* fb, int x, int y, int w, int h);
    SDK_EXPORT NVGLUframebuffer* nanoVGCreateFramebuffer(NVGcontext* ctx, int w, int h, int imageFlags);
    SDK_EXPORT void nanoVGBindFramebuffer(NVGLUframebuffer* fb);
    SDK_EXPORT void nanoVGDeleteFramebuffer(NVGLUframebuffer* fb);
    SDK_EXPORT int nanoVGGetFramebufferImage(NVGLUframebuffer* fb);

#ifdef __cplusplus
}
#endif
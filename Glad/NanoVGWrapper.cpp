/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "NanoVGWrapper.h"

#if defined(NANOVG_GL3_IMPLEMENTATION)
#include "gl.h"
#elif defined(NANOVG_METAL_IMPLEMENTATION)
#include "nanovg_mtl.h"
#endif
#include "nanovg.h"

#if defined(NANOVG_GL3_IMPLEMENTATION)
#include "gl.h"
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"
#elif defined(NANOVG_METAL_IMPLEMENTATION)
#include "nanovg_mtl.h"
#endif

extern "C" NVGcontext* createNanoVGContext(int flags)
{
#if defined(NANOVG_GL3_IMPLEMENTATION)
    return nvgCreateGL3(flags);
#elif defined(NANOVG_METAL_IMPLEMENTATION)
    return nvgCreateMTL(flags);
#else
    return nullptr;
#endif
}

extern "C" void destroyNanoVGContext(NVGcontext* ctx)
{
#if defined(NANOVG_GL3_IMPLEMENTATION)
    nvgDeleteGL3(ctx);
#elif defined(NANOVG_METAL_IMPLEMENTATION)
    nvgDeleteMTL(ctx);
#endif
}

extern "C" void nanoVGBlitFramebuffer(NVGcontext* ctx, NVGLUframebuffer* fb, int x, int y, int w, int h)
{
    nvgluBlitFramebuffer(ctx, fb, x, y, w, h);
}

extern "C" NVGLUframebuffer* nanoVGCreateFramebuffer(NVGcontext* ctx, int w, int h, int imageFlags)
{
    return nvgluCreateFramebuffer(ctx, w, h, imageFlags);
}

extern "C" void nanoVGBindFramebuffer(NVGLUframebuffer* fb)
{
    nvgluBindFramebuffer(fb);
}

extern "C" void nanoVGDeleteFramebuffer(NVGLUframebuffer* fb)
{
    nvgluDeleteFramebuffer(fb);
}

extern "C" int nanoVGGetFramebufferImage(NVGLUframebuffer* fb)
{
    return fb->image;
}

extern "C" void nanoVGStencilMaskTiles(int drawableW, int drawableH, int tileSize, std::span<const uint64_t> dirtyTiles)
{
    nvgluStencilMaskTiles(drawableW, drawableH, tileSize, dirtyTiles);
}

extern "C" void nanoVGForceStencilFill(NVGLUframebuffer* framebuffer, int width, int height) {
    nvgluVGForceStencilFill(framebuffer, width, height);
}
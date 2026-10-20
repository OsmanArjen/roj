#pragma once
#include <algorithm>
#include <cstdint>

#include <bgfx/bgfx.h>

namespace roj {

    template <typename Handle>
    constexpr Handle gfxInvalid() {
        return Handle{ bgfx::kInvalidHandle };
    }

    template <typename Handle>
    inline void gfxDestroyHandle(Handle h) {
        if (bgfx::isValid(h))
            bgfx::destroy(h);
    }

    // Destroy + invalidate: safe to call twice.
    template <typename Handle>
    inline void gfxRelease(Handle& h) {
        gfxDestroyHandle(h);
        h = gfxInvalid<Handle>();
    }

    // bgfx view rects are uint16_t; a bare cast silently wraps.
    inline uint16_t gfxToU16(int v) {
        return uint16_t(std::clamp(v, 0, 65535));
    }

    inline bgfx::TextureHandle gfxCreateSolidTexture(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        const uint8_t texel[4] = { r, g, b, a };
        return bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_NONE, bgfx::copy(texel, sizeof(texel)));
    }

}
#pragma once
#include <cstdint>

#include <bgfx/bgfx.h>

#include <entt/fwd.hpp>

#include <glm/glm.hpp>
#include <string>
namespace roj {
    constexpr uint16_t kGfxMaxJoints = 128;
    struct GfxInit {
        void* nwh = nullptr;
        glm::ivec2 resolution{ 1280, 720 };
        bgfx::RendererType::Enum type = bgfx::RendererType::Vulkan;
        const char* shaderRoot = "..\\..\\..\\assets\\shaders\\compiled\\";
    };
    
    struct Camera;
    struct GfxView {
        uint16_t    id = bgfx::kInvalidHandle;   // assigned in gfxInitialize
        const char* name{ nullptr };
        glm::ivec2  origin{ 0 };
        glm::ivec2  size{ 0 };
        glm::mat4   view = glm::mat4(1.0f);       // NEW
        glm::mat4   projection = glm::mat4(1.0f);
        glm::vec3   eye{ 0.0f };

        bgfx::FrameBufferHandle target = BGFX_INVALID_HANDLE;

        uint16_t    clearFlags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;
        uint32_t    clearRgba = 0x303030FF;
        float       clearDepth = 1.0f;
        uint8_t     clearStencil = 0;
    };

    struct GfxContext {
        GfxView forwardView;
        bgfx::ProgramHandle forwardStatic = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle forwardSkinned = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle u_albedo = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle u_normal = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle u_metallicroughness = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle u_emissive = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle u_camPos = BGFX_INVALID_HANDLE;
		bgfx::UniformHandle u_joints = BGFX_INVALID_HANDLE;

        bgfx::TextureHandle fallbackAlbedo = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle fallbackNormal = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle fallbackMetalRough = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle fallbackEmissive = BGFX_INVALID_HANDLE;

        uint64_t forwardState = 0;
    };

    std::string gfxShaderPath(const char* root, const char* stage, const char* name);

    bgfx::ShaderHandle  gfxLoadShader(const char* path);
    bgfx::ProgramHandle gfxLoadProgram(const char* vsPath, const char* fsPath);

    bool gfxInitialize(entt::registry& reg, GfxContext& r, const GfxInit& gi);
    void gfxSetViewCamera(GfxView& v, const Camera& cam);   // NEW
    void gfxSubmit(GfxContext& r, entt::registry& registry);
    void gfxShutdown(GfxContext& r, entt::registry& registry);


}
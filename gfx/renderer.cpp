#include <gfx/renderer.h>
#include <gfx/resource/mesh.h>
#include <gfx/resource/material.h>

#include <app/log.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <bx/error.h>
#include <bx/file.h>
#include <bx/readerwriter.h>

#include <bimg/bimg.h>

#include <gfx/common.h>

#include <entt/entity/registry.hpp>
#include <entity/transform.h>
#include <entity/camera.h>
#include <entity/animation.h>

namespace roj {
    namespace {

        enum : uint8_t { kStageAlbedo = 0, kStageNormal, kStageMetalRough, kStageEmissive, kStageOcclusion };

        bgfx::TextureHandle createBgfxTexture(const std::shared_ptr<bimg::ImageContainer>& image) {
            if (!image)
                return BGFX_INVALID_HANDLE;

            const bimg::ImageContainer& img = *image;

            if (img.m_offset != 0 || img.m_data == nullptr)
                return BGFX_INVALID_HANDLE;


            const bgfx::Caps::Limits& limits = bgfx::getCaps()->limits;
            uint32_t fullChain = 1;
            for (uint32_t side = std::max({ img.m_width, img.m_height, img.m_depth }); side > 1; side >>= 1)
                ++fullChain;
            if (img.m_width > limits.maxTextureSize || img.m_height > limits.maxTextureSize ||
                (img.m_numMips > 1 && img.m_numMips != fullChain))
                return BGFX_INVALID_HANDLE;

            const auto format = static_cast<bgfx::TextureFormat::Enum>(img.m_format);
            const bool volume = !img.m_cubeMap && img.m_depth > 1;
            const bool hasMips = img.m_numMips > 1;
            const auto width = static_cast<uint16_t>(img.m_width);
            const auto height = static_cast<uint16_t>(img.m_height);
            const auto depth = static_cast<uint16_t>(volume ? img.m_depth : 0);
            const uint16_t layers = img.m_numLayers;
            const uint64_t flags = img.m_srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE;

            // Format support (including the sRGB view) and the layer limit.
            if (!bgfx::isTextureValid(depth, img.m_cubeMap, layers, format, flags))
                return BGFX_INVALID_HANDLE;

            // ReleaseFn gets one void*, a shared_ptr is two pointers, hence the heap copy.
            const bgfx::Memory* mem = bgfx::makeRef(
                img.m_data, img.m_size,
                [](void*, void* ref) { delete static_cast<std::shared_ptr<bimg::ImageContainer>*>(ref); },
                new std::shared_ptr<bimg::ImageContainer>(image));

            if (img.m_cubeMap)
                return bgfx::createTextureCube(width, hasMips, layers, format, flags, mem);
            if (volume)
                return bgfx::createTexture3D(width, height, depth, hasMips, format, flags, mem);
            return bgfx::createTexture2D(width, height, hasMips, layers, format, flags, mem);
        }

        // Loader: uploads the image and adds the handle.
        void onImageConstruct(entt::registry& reg, entt::entity entity) {
            const bgfx::TextureHandle handle = createBgfxTexture(reg.get<std::shared_ptr<bimg::ImageContainer>>(entity));
            if (!bgfx::isValid(handle))
                return;
            reg.remove<bgfx::TextureHandle>(entity); // a stale one would otherwise be overwritten without being destroyed
            reg.emplace<bgfx::TextureHandle>(entity, handle);
        }

        void onImageDestroy(entt::registry& reg, entt::entity entity) {
            reg.remove<bgfx::TextureHandle>(entity);
        }


        void onTextureDestroy(entt::registry& reg, entt::entity entity) {
            const bgfx::TextureHandle handle = reg.get<bgfx::TextureHandle>(entity);
            if (bgfx::isValid(handle))
                bgfx::destroy(handle);
        }

        bgfx::TextureHandle resolveTexture(entt::registry& reg, entt::entity e, bgfx::TextureHandle fallback) {
            if (e == entt::null)
                return fallback;
            const bgfx::TextureHandle* t = reg.try_get<bgfx::TextureHandle>(e);
            return (t && bgfx::isValid(*t)) ? *t : fallback;
        }

    } 

    bool gfxInitialize(entt::registry& reg,GfxContext& r, const GfxInit& gi) {
        bgfx::Init bi{};
        bi.type = gi.type;
        bi.swapChain.width = gi.resolution.x;
        bi.swapChain.height = gi.resolution.y;
        bi.swapChain.nwh = gi.nwh;
        bi.reset = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X16 | BGFX_RESET_MAXANISOTROPY;
        if (!bgfx::init(bi)) {
            logWrite(LogError, "bgfx::init failed");
            return false;
        }

        r.forwardStatic = gfxLoadProgram(
            gfxShaderPath("../../../assets/shaders/compiled", "vs", "static_forward").c_str(),
            gfxShaderPath("../../../assets/shaders/compiled", "fs", "static_forward").c_str());

        r.forwardSkinned = gfxLoadProgram(
            gfxShaderPath("../../../assets/shaders/compiled", "vs", "skinned_forward").c_str(),
            gfxShaderPath("../../../assets/shaders/compiled", "fs", "static_forward").c_str());

        if (!bgfx::isValid(r.forwardSkinned)) {
            logWrite(LogError, "skinned_forward program failed to load");
            bgfx::shutdown();
            return false;
        }

        if (!bgfx::isValid(r.forwardStatic)) {
            logWrite(LogError, "static_forward program failed to load");
            bgfx::shutdown();
            return false;
        }
        r.u_joints = bgfx::createUniform("u_joints", bgfx::UniformType::Mat4, kGfxMaxJoints);
        r.u_albedo = bgfx::createUniform("u_albedo", bgfx::UniformType::Sampler);
        r.u_normal = bgfx::createUniform("u_normal", bgfx::UniformType::Sampler);
        r.u_metallicroughness = bgfx::createUniform("u_metallicroughness", bgfx::UniformType::Sampler);
        r.u_emissive = bgfx::createUniform("u_emissive", bgfx::UniformType::Sampler);
        r.u_occlusion = bgfx::createUniform("u_occlusion", bgfx::UniformType::Sampler);
        r.u_camPos = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4);
        r.u_baseColorFactor = bgfx::createUniform("u_baseColorFactor", bgfx::UniformType::Vec4);
        r.u_emissiveFactor = bgfx::createUniform("u_emissiveFactor", bgfx::UniformType::Vec4);
        r.u_materialParams = bgfx::createUniform("u_materialParams", bgfx::UniformType::Vec4);

        r.fallbackAlbedo = gfxCreateSolidTexture(255, 255, 255, 255);  // x albedo = unchanged
        r.fallbackNormal = gfxCreateSolidTexture(128, 128, 255, 255);  // tangent-space (0,0,1)
        r.fallbackMetalRough = gfxCreateSolidTexture(255, 255, 0, 255);  // G=rough 1, B=metal 0
        r.fallbackEmissive = gfxCreateSolidTexture(0, 0, 0, 255);  // adds no light
		r.fallbackOcclusion = gfxCreateSolidTexture(255, 255, 255, 255);  // adds no occlusion
        r.forwardView.id = 0;
        r.forwardView.name = "forward";
        r.forwardState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA;

        reg.on_construct<std::shared_ptr<bimg::ImageContainer>>().connect<&onImageConstruct>();
        reg.on_destroy<std::shared_ptr<bimg::ImageContainer>>().connect<&onImageDestroy>();
        reg.on_destroy<bgfx::TextureHandle>().connect<&onTextureDestroy>();
        return true;
    }

    void gfxSubmit(GfxContext& r, entt::registry& registry) {
        const GfxView& v = r.forwardView;
        if (v.size.x <= 0 || v.size.y <= 0)
            return;                                  // minimized: nothing to draw into

        if (v.name) bgfx::setViewName(v.id, v.name);
        bgfx::setViewRect(v.id, uint16_t(v.origin.x), uint16_t(v.origin.y),
            uint16_t(v.size.x), uint16_t(v.size.y));
        bgfx::setViewClear(v.id, v.clearFlags, v.clearRgba, v.clearDepth, v.clearStencil);
        bgfx::setViewFrameBuffer(v.id, v.target);
        bgfx::setViewTransform(v.id, glm::value_ptr(v.view), glm::value_ptr(v.projection));
        bgfx::touch(v.id);                           // run the clear even if nothing is drawn

        const glm::vec4 camPos(v.eye, 1.0f);

        auto objects = registry.view<GfxMeshInstance, GfxPbrMaterial, Transform::GlobalMatrix>();
        for (auto&& [entity, instance, material, global] : objects.each()) {
            const GfxMeshHandle* mesh = registry.try_get<GfxMeshHandle>(instance.id);
            if (!mesh || !mesh->valid())
                continue;                            // not uploaded yet (or destroyed): skip quietly

            const glm::mat4 model = global;
            bgfx::setTransform(glm::value_ptr(model));   // bgfx copies the 16 floats now
            bgfx::setVertexBuffer(0, bgfx::VertexBufferHandle{ mesh->vbh });
            bgfx::setIndexBuffer(bgfx::IndexBufferHandle{ mesh->ibh });

            bgfx::setTexture(kStageAlbedo, r.u_albedo,
                resolveTexture(registry, material.albedo, r.fallbackAlbedo));
            bgfx::setTexture(kStageNormal, r.u_normal,
                resolveTexture(registry, material.normal, r.fallbackNormal));
            bgfx::setTexture(kStageMetalRough, r.u_metallicroughness,
                resolveTexture(registry, material.metallicRoughness, r.fallbackMetalRough));
            bgfx::setTexture(kStageEmissive, r.u_emissive,
                resolveTexture(registry, material.emissive, r.fallbackEmissive));
            bgfx::setTexture(kStageOcclusion, r.u_occlusion,
                resolveTexture(registry, material.occlusion, r.fallbackOcclusion));

			glm::vec4 materialParams(material.metallicFactor, material.roughnessFactor, material.normalScale, material.occlusionStrength);
            bgfx::setUniform(r.u_baseColorFactor, glm::value_ptr(material.baseColor));
            bgfx::setUniform(r.u_emissiveFactor, glm::value_ptr(material.emissiveFactor));
            bgfx::setUniform(r.u_materialParams, glm::value_ptr(materialParams));

            const SkinPalette* skin = registry.try_get<SkinPalette>(entity);
            const bool skinned = skin && !skin->matrices.empty();
            if (skinned) {
                const uint16_t count = uint16_t(std::min<size_t>(skin->matrices.size(), kGfxMaxJoints));
                bgfx::setUniform(r.u_joints, skin->matrices.data(), count);
            }

            bgfx::setUniform(r.u_camPos, &camPos);
            bgfx::setState(r.forwardState);
            bgfx::submit(v.id, skinned ? r.forwardSkinned : r.forwardStatic);
        }
        bgfx::frame();

    }

    void gfxShutdown(GfxContext& r, entt::registry& registry) {
		registry.clear<roj::GfxMeshData>();
        gfxDestroyHandle(r.fallbackAlbedo);
        gfxDestroyHandle(r.fallbackNormal);
        gfxDestroyHandle(r.fallbackMetalRough);
        gfxDestroyHandle(r.fallbackEmissive);
        gfxDestroyHandle(r.u_albedo);
        gfxDestroyHandle(r.u_normal);
        gfxDestroyHandle(r.u_metallicroughness);
        gfxDestroyHandle(r.u_emissive);
        gfxDestroyHandle(r.u_camPos);
        gfxDestroyHandle(r.u_joints);
        gfxDestroyHandle(r.forwardStatic);
        gfxDestroyHandle(r.forwardSkinned);
        r = GfxContext{};                            // no dangling handles left behind
        bgfx::shutdown();
    }



    namespace {

        const char* shaderPlatformDir() {
#if defined(_WIN32)
            return "windows/";
#elif defined(__APPLE__)
            return "osx/";
#elif defined(__EMSCRIPTEN__)
            return "web/";
#elif defined(__ANDROID__)
            return "android/";
#elif defined(__linux__)
            return "linux/";
#else
            return "unknown/";
#endif
        }

        const char* shaderRendererDir(bgfx::RendererType::Enum type) {
            switch (type) {
            case bgfx::RendererType::Direct3D11: return "dx11/";
            case bgfx::RendererType::Direct3D12: return "dx12/";
            case bgfx::RendererType::OpenGL:     return "gl/";
            case bgfx::RendererType::OpenGLES:   return "gles/";
            case bgfx::RendererType::Vulkan:     return "spirv/";
            case bgfx::RendererType::Metal:      return "metal/";
            case bgfx::RendererType::WebGPU:     return "wgpu/";
            default:                             return "unknown/";
            }
        }

        // Reads the file into `out` plus `extra` zeroed bytes of tail padding. Reading
        // into plain memory first means a failed read can't leak a bgfx::alloc block
        // (bgfx only frees memory that has been handed back to it).
        bool readFile(const char* path, std::vector<uint8_t>* out, uint32_t extra, uint32_t* outSize) {
            bx::FileReader reader;
            if (!bx::open(&reader, path)) {
                logWrite(LogError, "Failed to open shader file: %s", path);
                return false;
            }

            const int64_t fileSize = bx::getSize(&reader);
            if (fileSize <= 0) {
                bx::close(&reader);
                logWrite(LogError, "Shader file is empty: %s", path);
                return false;
            }

            const uint32_t size = uint32_t(fileSize);
            out->assign(size_t(size) + extra, 0);
            bx::Error err;
            const int32_t got = bx::read(&reader, out->data(), int32_t(size), &err);
            bx::close(&reader);
            if (!err.isOk() || got != int32_t(size)) {
                logWrite(LogError, "Failed to read shader file: %s", path);
                return false;
            }
            *outSize = size;
            return true;
        }

    } // anonymous namespace

    std::string gfxShaderPath(const char* root, const char* stage, const char* name) {
        std::string path(root ? root : "");
        if (!path.empty() && path.back() != '/' && path.back() != '\\')
            path += '/';
        path += shaderPlatformDir();
        path += shaderRendererDir(bgfx::getRendererType());
        path += stage;
        path += '_';
        path += name;
        path += ".bin";
        return path;
    }

    bgfx::ShaderHandle gfxLoadShader(const char* path) {
        std::vector<uint8_t> bytes;
        uint32_t size = 0;
        if (!readFile(path, &bytes, 1, &size))
            return gfxInvalid<bgfx::ShaderHandle>();

        bgfx::ShaderHandle handle = bgfx::createShader(bgfx::copy(bytes.data(), size + 1));
        if (!bgfx::isValid(handle)) {
            logWrite(LogError, "bgfx rejected shader: %s", path);
            return gfxInvalid<bgfx::ShaderHandle>();
        }
        bgfx::setName(handle, path);
        return handle;
    }

    bgfx::ProgramHandle gfxLoadProgram(const char* vsPath, const char* fsPath) {
        bgfx::ShaderHandle vsh = gfxLoadShader(vsPath);
        bgfx::ShaderHandle fsh = gfxLoadShader(fsPath);
        if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
            gfxDestroyHandle(vsh);
            gfxDestroyHandle(fsh);
            return gfxInvalid<bgfx::ProgramHandle>();
        }
        return bgfx::createProgram(vsh, fsh, true);
    }

} // namespace roj
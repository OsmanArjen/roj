#include <gfx/resource/mesh.h>

#include <algorithm>
#include <cstring>

#include <bgfx/bgfx.h>

#include <entt/entity/registry.hpp>

#include <app/log.h>
#include <gfx/common.h>

namespace roj {

    namespace {

        bool toBgfxAttribType(GfxVertexFormat f, bgfx::AttribType::Enum* out) {
            using T = bgfx::AttribType;
            switch (f) {
            case GfxVertexFormat::Uint8:  *out = T::Uint8;  return true;
            case GfxVertexFormat::Uint10: *out = T::Uint10; return true;
            case GfxVertexFormat::Int16:  *out = T::Int16;  return true;
            case GfxVertexFormat::Half:   *out = T::Half;   return true;
            case GfxVertexFormat::Float:  *out = T::Float;  return true;
            case GfxVertexFormat::Int8:   *out = T::Int8;   return true;
            case GfxVertexFormat::Uint16: *out = T::Uint16; return true;
            case GfxVertexFormat::Int32:  *out = T::Int32;  return true;
            case GfxVertexFormat::Uint32: *out = T::Uint32; return true;
            default:
                return false;   
            }
            return false;
        }

        bool toBgfxAttrib(GfxVertexSemantic s, bgfx::Attrib::Enum* out) {
            using A = bgfx::Attrib;
            switch (s) {
            case GfxVertexSemantic::Position:  *out = A::Position;  return true;
            case GfxVertexSemantic::Normal:    *out = A::Normal;    return true;
            case GfxVertexSemantic::Tangent:   *out = A::Tangent;   return true;
            case GfxVertexSemantic::Bitangent: *out = A::Bitangent; return true;
            case GfxVertexSemantic::Color0:    *out = A::Color0;    return true;
            case GfxVertexSemantic::Color1:    *out = A::Color1;    return true;
            case GfxVertexSemantic::Color2:    *out = A::Color2;    return true;
            case GfxVertexSemantic::Color3:    *out = A::Color3;    return true;
            case GfxVertexSemantic::Indices:   *out = A::Indices;   return true;
            case GfxVertexSemantic::Weight:    *out = A::Weight;    return true;
            case GfxVertexSemantic::TexCoord0: *out = A::TexCoord0; return true;
            case GfxVertexSemantic::TexCoord1: *out = A::TexCoord1; return true;
            case GfxVertexSemantic::TexCoord2: *out = A::TexCoord2; return true;
            case GfxVertexSemantic::TexCoord3: *out = A::TexCoord3; return true;
            case GfxVertexSemantic::TexCoord4: *out = A::TexCoord4; return true;
            case GfxVertexSemantic::TexCoord5: *out = A::TexCoord5; return true;
            case GfxVertexSemantic::TexCoord6: *out = A::TexCoord6; return true;
            case GfxVertexSemantic::TexCoord7: *out = A::TexCoord7; return true;
            default:
                return false;   
            }
        }

        bool buildLayout(const std::vector<GfxVertexAttribute>& attrs, bgfx::VertexLayout* out) {
            if (attrs.empty()) {
                logWrite(LogError, "Mesh: empty vertex layout");
                return false;
            }
            out->begin();
            for (const GfxVertexAttribute& a : attrs) {
                bgfx::Attrib::Enum attrib;
                bgfx::AttribType::Enum type;
                if (!toBgfxAttrib(a.type, &attrib)) {
                    logWrite(LogError, "Mesh: vertex semantic %d has no bgfx attribute", (int)a.type);
                    return false;
                }
                if (!toBgfxAttribType(a.format, &type)) {
                    logWrite(LogError, "Mesh: vertex format %d has no bgfx attribute type", (int)a.format);
                    return false;
                }
                if (a.component < 1 || a.component > 4) {
                    logWrite(LogError, "Mesh: attribute component count %d out of range 1..4", (int)a.component);
                    return false;
                }
                out->add(attrib, a.component, type, a.normalized);
            }
            out->end();
            return true;
        }

        void freeMesh(entt::registry& reg, entt::entity id) {
            const GfxMeshHandle* h = reg.try_get<GfxMeshHandle>(id);
            if (!h)
                return;
            gfxDestroyHandle(bgfx::VertexBufferHandle{ h->vbh });
            gfxDestroyHandle(bgfx::IndexBufferHandle{ h->ibh });
            reg.remove<GfxMeshHandle>(id);   // never leave a dangling handle behind
        }

        // All validation happens BEFORE the first bgfx::alloc: memory handed out by
        // alloc but never passed to bgfx is leaked, so a late failure must not exist.
        void uploadMesh(entt::registry& reg, entt::entity id) {
            const GfxMeshData& data = reg.get<GfxMeshData>(id);
            const uint32_t eid = (uint32_t)entt::to_integral(id);

            bgfx::VertexLayout layout;
            if (!buildLayout(data.layout, &layout)) {
                logWrite(LogError, "Mesh (entity %u): upload skipped", eid);
                return;
            }

            const uint32_t stride = layout.getStride();
            if (stride == 0 || data.vertices.empty() || data.vertices.size() % stride != 0) {
                logWrite(LogError, "Mesh (entity %u): %zu vertex bytes is not a positive multiple of stride %u",
                    eid, data.vertices.size(), (unsigned)stride);
                return;
            }
            if (data.indices.empty()) {
                logWrite(LogError, "Mesh (entity %u): no indices", eid);
                return;
            }

            const uint32_t vertexCount = uint32_t(data.vertices.size() / stride);
            const uint32_t indexCount = uint32_t(data.indices.size());
            const uint32_t maxIndex = *std::max_element(data.indices.begin(), data.indices.end());
            if (maxIndex >= vertexCount) {
                logWrite(LogError, "Mesh (entity %u): index %u out of range (%u vertices)",
                    eid, (unsigned)maxIndex, (unsigned)vertexCount);
                return;
            }

            // bgfx::copy, not makeRef: a reference would dangle the moment the CPU copy
            // is freed or reallocated, and bgfx reads it on the render thread later.
            const bgfx::Memory* vmem = bgfx::copy(data.vertices.data(), uint32_t(data.vertices.size()));

            // Meshes that fit in 16 bits get 16-bit indices: half the index bandwidth.
            const bool narrow = vertexCount <= 0x10000u;
            const bgfx::Memory* imem = nullptr;
            uint16_t indexFlags = BGFX_BUFFER_NONE;
            if (narrow) {
                imem = bgfx::alloc(indexCount * uint32_t(sizeof(uint16_t)));
                uint16_t* dst = reinterpret_cast<uint16_t*>(imem->data);
                for (uint32_t i = 0; i < indexCount; ++i)
                    dst[i] = uint16_t(data.indices[i]);
            }
            else {
                imem = bgfx::copy(data.indices.data(), indexCount * uint32_t(sizeof(uint32_t)));
                indexFlags = BGFX_BUFFER_INDEX32;
            }

            const bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(vmem, layout);
            const bgfx::IndexBufferHandle  ibh = bgfx::createIndexBuffer(imem, indexFlags);
            if (!bgfx::isValid(vbh) || !bgfx::isValid(ibh)) {
                gfxDestroyHandle(vbh);
                gfxDestroyHandle(ibh);
                logWrite(LogError, "Mesh (entity %u): bgfx buffer creation failed", eid);
                return;
            }

            reg.emplace_or_replace<GfxMeshHandle>(id, GfxMeshHandle{ vbh.idx, ibh.idx });
        }

    } // anonymous namespace

    void GfxMeshData::on_construct(entt::registry& reg, entt::entity id) {
        uploadMesh(reg, id);
    }

    void GfxMeshData::on_update(entt::registry& reg, entt::entity id) {
        freeMesh(reg, id);
        uploadMesh(reg, id);
    }

    void GfxMeshData::on_destroy(entt::registry& reg, entt::entity id) {
        freeMesh(reg, id);
    }

}
#pragma once
#include <charconv>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include <glm/mat4x4.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <entt/container/dense_map.hpp>
#include <assimp/material.h>
#include <gfx/resource/mesh.h>
#include <entity/animation.h>
namespace roj {

    // Properties the importer adds to every ModelAsset material that references an embedded texture.
// Property *index* = texture index in aiScene::mTextures (the N of a "*N" path), so a lookup is
// (key, 0, N) with no string formatting, the same triple assimp uses for its own texture properties.
    inline constexpr const char* kEmbeddedSize = "$mat.embedded_size";    // int[2]  {mWidth, mHeight}
    inline constexpr const char* kEmbeddedFormat = "$mat.embedded_format";  // buffer  aiTexture::achFormatHint, verbatim
    inline constexpr const char* kEmbeddedData = "$mat.embedded_data";    // buffer  aiTexture::pcData, verbatim

    // Owned copy of one aiScene::mTextures entry; index in ModelAsset::embedded == the N of "*N".
    struct EmbeddedTexture {
        uint32_t width = 0, height = 0;  // height == 0: `data` is a compressed file image (png, jpg, ...), data.size() == width
        // height != 0: width*height aiTexels, 4 bytes each, in B,G,R,A order
        std::string format;              // assimp's hint: "png", "jpg", or "rgba8888" for raw texels
        std::vector<uint8_t> data;       // empty: the source texture had no data
    };

    struct NodeTable {
        std::vector<std::string> names;
        std::vector<entt::id_type> parents;
        std::vector<uint32_t> subtrees;        
        std::vector<glm::mat4> locals;
        std::vector<entt::id_type> meshes;     
        std::vector<entt::id_type> materials;  
        std::vector<uint8_t> skinned;          
    };

    struct ModelJoint { entt::id_type node; glm::mat4 inverseBindMatrix; };

    struct ModelAsset {
        ModelAsset() = default;
        ModelAsset(ModelAsset&&) noexcept = default;
        ModelAsset& operator=(ModelAsset&&) noexcept = default;
        ModelAsset(const ModelAsset&) = delete;
        ModelAsset& operator=(const ModelAsset&) = delete;
        std::string path, log;
        NodeTable nodes;                       
        std::vector<ModelJoint> joints;
        std::vector<GfxMeshData> meshes;
        std::vector<AnimationClip> animations;
        std::vector<std::shared_ptr<aiMaterial>> materials;
        std::vector<EmbeddedTexture> embedded;   // indexed by aiScene::mTextures index
    };

    // assimp's spelling of "aiScene::mTextures[N]". False for ordinary file paths.
    inline bool parseEmbeddedRef(const char* path, unsigned& index) {
        if (path[0] != '*') return false;
        const char* end = path + std::strlen(path);
        const auto [ptr, ec] = std::from_chars(path + 1, end, index);
        return ec == std::errc{} && ptr == end;
    }

    std::optional<ModelAsset> importModel(const std::filesystem::path& path);

}
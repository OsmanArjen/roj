#include <asset/resource.h>
#include <asset/texture.h>
#include <asset/scene.h>

#include <gfx/resource/mesh.h>

#include <entity/transform.h>
#include <entity/animation.h>

#include <entt/container/dense_map.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <algorithm>


namespace {

    using TextureResource = std::shared_ptr<bimg::ImageContainer>;

    entt::entity loadMaterialTexture(roj::ModelScene& model, entt::registry& reg, const std::filesystem::path& modelDir,
        const std::vector<roj::EmbeddedTexture>& embedded, std::shared_ptr<aiMaterial> aimat, aiTextureType type) {
        if (aimat->GetTextureCount(type) == 0)
            return entt::null;

        aiString name;
        aimat->GetTexture(type, 0, &name);

        // One key scheme for both kinds: "*N" is already a unique name within the model.
        const auto strkey = model.path + ":" + name.C_Str();
        const entt::id_type key = entt::hashed_string(strkey.c_str());

        // Several materials may reference the same image; the key makes them share one resource.
        if (const auto cached = model.textures.find(key); cached != model.textures.end())
            return cached->second;

        entt::entity texture = entt::null;
        unsigned index;
        if (roj::parseEmbeddedRef(name.C_Str(), index)) {
            if (index >= embedded.size() || embedded[index].data.empty()) {
                model.log += "Missing embedded texture data for " + std::string(name.C_Str()) + "\n";
                return entt::null;
            }
            texture = roj::resourceLoad<TextureResource>(reg, key, std::cref(embedded[index]));
        }
        else {
            texture = roj::resourceLoad<TextureResource>(reg, key, modelDir / name.C_Str());
        }

        model.textures.emplace(key, texture);
        return texture;
    }

    roj::GfxPbrMaterial convertMaterial(roj::ModelScene& model, entt::registry& reg, const std::vector<roj::EmbeddedTexture>& embedded, std::shared_ptr<aiMaterial> aimat) {
        const std::filesystem::path modelDir = std::filesystem::path(model.path).parent_path();
        auto load = [&](aiTextureType type) {
            return loadMaterialTexture(model, reg, modelDir, embedded, aimat, type);
            };

        roj::GfxPbrMaterial material;
        material.albedo = load(aiTextureType_BASE_COLOR);
        material.normal = load(aiTextureType_NORMALS);
        material.metallicRoughness = load(aiTextureType_GLTF_METALLIC_ROUGHNESS);
        material.emissive = load(aiTextureType_EMISSIVE);
        material.occlusion = load(aiTextureType_LIGHTMAP);
        aiColor4D color;
        if (aimat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
            material.baseColor = { color.r, color.g, color.b, color.a };

        aiColor3D emissive;
        if (aimat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
            material.emissiveFactor = { emissive.r, emissive.g, emissive.b };

        aimat->Get(AI_MATKEY_METALLIC_FACTOR, material.metallicFactor);
        aimat->Get(AI_MATKEY_ROUGHNESS_FACTOR, material.roughnessFactor);


        return material;
    }

    // ---------------------------------------------------------------------------
    // Scene loading steps (called in order by loadModel)
    // ---------------------------------------------------------------------------

    void loadMeshes(roj::ModelScene& scene, entt::registry& reg, const roj::ModelAsset& asset) {
        for (uint32_t i = 0; i < asset.meshes.size(); i++){
            scene.meshes[i] = reg.create();
            reg.emplace<roj::GfxMeshData>(scene.meshes[i], std::move(asset.meshes[i]));
        }
    }
    void loadAnimations(roj::ModelScene& scene, entt::registry& reg, const roj::ModelAsset& asset) {
        for (uint32_t i = 0; i < asset.animations.size(); i++) {
            scene.animations[i] = reg.create();
            reg.emplace<roj::AnimationClip>(scene.animations[i], std::move(asset.animations[i]));
        }
    }
    void loadMaterials(roj::ModelScene& scene, entt::registry& reg, const roj::ModelAsset& asset) {
        for (uint32_t i = 0; i < asset.materials.size(); i++) {
            scene.materials[i] = convertMaterial(scene, reg, asset.embedded, asset.materials[i]);
        }
    }


    // ---------------------------------------------------------------------------
    // Instantiation
    // ---------------------------------------------------------------------------

    roj::Transform decomposeLocal(const glm::mat4& local) {
        roj::Transform transform{};
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(local, transform.scale, transform.rotation, transform.position, skew, perspective);
        return transform;
    }
    std::vector<entt::entity> instantiateNodes(entt::registry& reg, const roj::ModelScene& model,
        entt::entity rootEntity) {
        const roj::NodeTable& nodes = model.nodes;

        // Row 0 is the caller's entity; every other row gets a fresh one.
        std::vector<entt::entity> nodeEntities(nodes.names.size());
        nodeEntities[0] = rootEntity;
        reg.create(nodeEntities.begin() + 1, nodeEntities.end());

        // Parents precede children in the table, so attaching in row order always finds the parent in place.
        for (size_t row = 0; row < nodes.names.size(); row++) {
            const entt::entity entity = nodeEntities[row];
            const entt::entity parentEntity = nodes.parents[row] == entt::null
                ? entt::entity{ entt::null }
            : nodeEntities[nodes.parents[row]];

            reg.emplace<roj::Transform>(entity, decomposeLocal(nodes.locals[row]));
            roj::Transform::attach(reg, entity, parentEntity);

            if (nodes.meshes[row] != entt::null) {
                reg.emplace<roj::GfxMeshInstance>(entity, model.meshes[nodes.meshes[row]]);
                if (nodes.skinned[row])
                    reg.emplace<roj::SkinPalette>(entity, roj::SkinPalette{ rootEntity, std::vector<glm::mat4>(model.joints.size(), glm::mat4(1.0f)) });
                if (nodes.materials[row] != entt::null)
                    reg.emplace<roj::GfxPbrMaterial>(entity, model.materials[nodes.materials[row]]);
            }
        }
        return nodeEntities;
    }

} // namespace

namespace roj {

    void instantiateModel(entt::handle root, const ModelScene& model) {
        if (model.nodes.names.empty())
            return;
        entt::registry& reg = *root.registry();

        std::vector<entt::entity> nodeEntities = instantiateNodes(reg, model, root.entity());

        if (model.animations.empty() && model.joints.empty())
            return; // static models don't pay for a rig

        std::vector<RigJoint> joints;
        joints.reserve(model.joints.size());
        for (const ModelJoint& joint : model.joints)
            joints.push_back({ joint.node, joint.inverseBindMatrix });

        reg.emplace<Rig>(root.entity(), makeRig(reg, std::move(nodeEntities), std::move(joints)));
    }

    ModelScene loadModel(entt::registry& reg, ModelAsset& asset) {


        ModelScene scene;
        scene.path = asset.path;                 // materials build their texture keys from this, so it must be set first
        scene.log = asset.log;
        scene.nodes = std::move(asset.nodes);
        scene.joints = std::move(asset.joints);
        scene.meshes.resize(asset.meshes.size());
        scene.materials.resize(asset.materials.size());
        scene.animations.resize(asset.animations.size());

        loadMeshes(scene, reg, asset);
        loadAnimations(scene, reg, asset);
        loadMaterials(scene, reg, asset);
        

        return scene;
    }

} // namespace roj
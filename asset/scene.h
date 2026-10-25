#pragma once
#include <gfx/resource/material.h>
#include <asset/import.h>
namespace roj {

    struct ModelScene {
        std::string path, log;
        NodeTable nodes;                       // empty means "no model"; the root is row 0
        std::vector<ModelJoint> joints;
        std::vector<entt::entity> meshes;
        std::vector<entt::entity> animations;
        std::vector<GfxPbrMaterial> materials;
        entt::dense_map<entt::id_type, entt::entity> textures;
    };

    void instantiateModel(entt::handle entity, const ModelScene& model);
    ModelScene loadModel(entt::registry& reg, ModelAsset& asset);
}
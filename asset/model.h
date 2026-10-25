#pragma once
#include <string>
#include <vector>
#include <glm/mat4x4.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <entt/container/dense_map.hpp>
#include <gfx/resource/material.h>
namespace roj {

    


    struct ModelNode {
        entt::id_type mesh;
        entt::id_type material;
		bool skinned = false;
        std::string name;
        glm::mat4 local;
        ModelNode* parent = nullptr;
        std::vector<ModelNode*> children;
    };

    struct ModelJoint { entt::id_type node; glm::mat4 inverseBindMatrix; };

    struct ModelScene {
        ModelNode* root;
        std::string path, log;
        std::vector<ModelNode> nodes;
        std::vector<ModelJoint> joints;
        std::vector<entt::entity> meshes;
        std::vector<entt::entity> animations;
        std::vector<GfxPbrMaterial> materials;
        entt::dense_map<entt::id_type, entt::entity> textures;
    };

    void instantiateModel(entt::handle entity, const ModelScene& model);
    ModelScene loadModel(entt::registry& reg, const std::string& path);
}
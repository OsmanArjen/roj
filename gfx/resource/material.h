#pragma once
#include <entt/entity/entity.hpp>
namespace roj {

    struct GfxPbrMaterial {
        glm::vec4 baseColor = glm::vec4(1.0f);
        glm::vec3 emissiveFactor = glm::vec3(0.0f);

        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;

        entt::entity albedo = entt::null;
        entt::entity normal = entt::null;
        entt::entity metallicRoughness = entt::null;
        entt::entity emissive = entt::null;
        entt::entity occlusion = entt::null;

        float normalScale = 1.0f;
        float occlusionStrength = 1.0f;
    };
}
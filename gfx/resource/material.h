#pragma once
#include <entt/entity/fwd.hpp>
namespace roj {

    struct GfxPbrMaterial {
        entt::entity albedo;
        entt::entity normal;
        entt::entity metallicRoughness;
        entt::entity emissive;
    };
}
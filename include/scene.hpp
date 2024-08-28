#ifndef SCENE_HPP
#define SCENE_HPP
#include <PxPhysicsAPI.h>

#include <entt/entt.hpp>
#include "model.hpp"
#include "skinned_model.hpp"
#include "animator.hpp"
#include "shader.hpp"
#include "input.hpp"
#include <unordered_map>
#include "camera.hpp"
#include <vector>

struct PhysxHandle
{
    physx::PxFoundation* foundation;
    physx::PxPhysics* physics;
    physx::PxDefaultCpuDispatcher* dispatcher;
    physx::PxDefaultAllocator allocator;
    physx::PxDefaultErrorCallback errorCallback;
};

struct ResourceHandle
{
    std::unordered_map<std::string, std::vector<roj::Mesh>> models;
    std::unordered_map<std::string, roj::SkinnedModel> skinnedModels;
	std::unordered_map<std::string, roj::GLShaderObject> shaderObjects;
	std::unordered_map<std::string, uint32_t> textures;
};

namespace roj
{
    enum SceneFlags : std::uint32_t
    {
        DEFAULT = 0,
        PHYSX_FLAG = 1 << 0,
    };

    class AbstractScene
    {
    public:
        virtual ~AbstractScene() = default;

        virtual void update(float deltatime) = 0;
        virtual void render() = 0;

        virtual void keyCallback(roj::Keycode keycode, roj::InputAction action) = 0;
        virtual void mouseCallback(roj::MouseButton button, roj::InputAction action) = 0;
        virtual void cursorCallback(float xpos, float ypos) = 0;
    };
}
#endif //-SCENE_HPP

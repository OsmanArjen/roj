#ifndef SCENE_HPP
#define SCENE_HPP
#include <entt/entt.hpp>
#include "model.hpp"
#include "skinned_model.hpp"
#include "animator.hpp"
#include "shader.hpp"
#include "input.hpp"
#include <unordered_map>
#include "camera.hpp"
#include <vector>

struct LightData
{
	glm::vec3 position;
	float ambient;
	float diffuse;
	float specular;
};

struct GameResource
{
	std::vector<std::vector<roj::Mesh>> models;
	std::vector<roj::SkinnedModel> skinnedModels;
	std::unordered_map<std::string, roj::GLShaderObject> shaderObjects;
	std::unordered_map<std::string, uint32_t> textures;
};

namespace roj
{
	struct Scene;
	struct SceneBinds
	{
		std::function<void(roj::Scene&, GameResource&)> init;
		std::function<void(roj::Scene&, GameResource&)> render;
		std::function<void(roj::Scene&)> update;

		std::function<void(roj::Scene&, roj::Keycode, roj::InputAction)> keyCallback;
		std::function<void(roj::Scene&, roj::MouseButton, roj::InputAction)> mouseCallback;
		std::function<void(roj::Scene&, float, float)> cursorCallback;
	};

	struct Scene
	{
		float deltatime = 0.f;
		roj::Camera camera;
		entt::registry entities;
		std::vector<LightData> lights;
		SceneBinds binds;
	};
}
#endif //-SCENE_HPP

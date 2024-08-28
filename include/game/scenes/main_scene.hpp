#ifndef MAIN_SCENE_HPP
#define MAIN_SCENE_HPP
#include "scene.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "skybox.hpp"
#include "game/component/common.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

struct EntityTags
{
	int groupTagId;
};

struct EditorHandle
{
	int nextGroupTag = 0;
	std::unordered_map<std::string, int> enttTypes;
	entt::entity selectedEntity = entt::null;
	bool open = false;
};


class MainScene : public roj::AbstractScene
{
private:
	roj::Camera m_camera;
	entt::registry m_entities;
	physx::PxScene* m_physxScene;
	ResourceHandle& m_resources;
	PhysxHandle& m_physxHandle;
	
	EditorHandle m_editorHandle;
	roj::Skybox m_skybox;
	entt::entity m_sceneMap;
	entt::entity player;
private:
	uint32_t m_quadVAO;
	uint32_t m_gBuffer;
	uint32_t m_gPosition;
	uint32_t m_gNormal;
	uint32_t m_gAlbedoSpec;
private:
	void loadModels();
	void loadShaders();
	void initGBuffer();
	void initSceneMap();
	void initScenePhysics();
	void initPlayer();

	void updateInput(float deltatime);
	void renderNonScene();
	void renderImgui();
	void renderScene();


public:
	MainScene(ResourceHandle& resources, PhysxHandle& physxHandle);
	~MainScene();
	void update(float deltatime) override;
	void render() override;

	void keyCallback(roj::Keycode key, roj::InputAction action) override;
	void mouseCallback(roj::MouseButton button, roj::InputAction action) override;
	void cursorCallback(float xpos_p, float ypos_p) override;
};
#endif //-MAIN_SCENE_HPP

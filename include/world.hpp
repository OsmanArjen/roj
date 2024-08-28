#ifndef WORLD_HPP
#define WORLD_HPP
#include "scene.hpp"
#include <memory>

class GameWorld
{
private:
	uint32_t m_nextSceneId = 1;
	uint32_t m_activeScene = 0;
	ResourceHandle m_resources;
	PhysxHandle  m_physxHandle;
	std::unordered_map<uint32_t, std::unique_ptr<roj::AbstractScene>> m_scenes;
private:
	void initPhysx();
public:
	GameWorld();
	~GameWorld();

	void update(float deltatime);
	void render();

	void mouseCallback(roj::MouseButton button, roj::InputAction action);
	void keyCallback(roj::Keycode key, roj::InputAction action);
	void cursorCallback(double x, double y);

	template <typename scene_t>
	uint32_t addScene(roj::SceneFlags flags);
	void removeScene(uint32_t id);
	void setActiveScene(uint32_t scene);
};

template <typename scene_t>
uint32_t GameWorld::addScene(roj::SceneFlags flags)
{
	physx::PxSceneDesc sceneDesc(m_physxHandle.physics->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = m_physxHandle.dispatcher;
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	m_scenes[m_nextSceneId] = std::make_unique< scene_t>(m_resources, m_physxHandle);
	return m_nextSceneId++;
}

#endif //-WORLD_HPP

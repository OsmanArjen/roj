#include "world.hpp"
void GameWorld::init()
{
	for (auto& [_, scene] : m_scenes)
	{
		scene.binds.init(scene, m_resources);
	}
}

void GameWorld::update(float deltatime)
{
	m_scenes[m_activeScene].deltatime = deltatime;
	m_scenes[m_activeScene].binds.update(m_scenes[m_activeScene]);
}

void GameWorld::render()
{
	m_scenes[m_activeScene].binds.render(m_scenes[m_activeScene], m_resources);
}

uint32_t GameWorld::addScene(roj::SceneBinds bindings)
{
	m_scenes[m_nextSceneId].binds = bindings;
	return m_nextSceneId++;
}

void GameWorld::removeScene(uint32_t id)
{
	m_scenes.erase(id);
}

void GameWorld::mouseCallback(roj::MouseButton button, roj::InputAction action)
{
	m_scenes[m_activeScene].binds.mouseCallback(m_scenes.at(m_activeScene), button, action);
}

void GameWorld::keyCallback(roj::Keycode key, roj::InputAction action)
{
	m_scenes[m_activeScene].binds.keyCallback(m_scenes.at(m_activeScene), key, action);

}

void GameWorld::cursorCallback(double x, double y)
{
	m_scenes[m_activeScene].binds.cursorCallback(m_scenes.at(m_activeScene),x, y);

}

void GameWorld::setActiveScene(uint32_t scene)
{
	m_activeScene = scene;
}

roj::Scene& GameWorld::getActiveScene()
{
	return m_scenes.at(m_activeScene);
}
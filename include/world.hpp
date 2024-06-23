#ifndef WORLD_HPP
#define WORLD_HPP
#include "scene.hpp"

class GameWorld
{
private:
	uint32_t m_nextSceneId = 1;
	uint32_t m_activeScene = 0;
	GameResource m_resources;
	std::unordered_map<uint32_t, roj::Scene> m_scenes;
	
public:
	GameWorld() = default;
	void init();
	void mouseCallback(roj::MouseButton button, roj::InputAction action);
	void keyCallback(roj::Keycode key, roj::InputAction action);
	void cursorCallback(double x, double y);

	void update(float deltatime);
	void render();
	uint32_t addScene(roj::SceneBinds bindings);
	void removeScene(uint32_t id);

	void setActiveScene(uint32_t scene);
	roj::Scene& getActiveScene();


};
#endif //-WORLD_HPP

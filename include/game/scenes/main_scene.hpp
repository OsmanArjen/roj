#ifndef MAIN_SCENE_HPP
#define MAIN_SCENE_HPP
#include "scene.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "game/component/common.hpp"

namespace mainscene
{

	void init(roj::Scene& scene, GameResource& resources);
	void update(roj::Scene& scene);
	void render(roj::Scene& scene, GameResource& resources);

	void keyCallback(roj::Scene& scene, roj::Keycode key, roj::InputAction action);
	void mouseCallback(roj::Scene& scene, roj::MouseButton button, roj::InputAction action);
	void cursorCallback(roj::Scene& scene, float xpos_p, float ypos_p);

}
#endif //-MAIN_SCENE_HPP

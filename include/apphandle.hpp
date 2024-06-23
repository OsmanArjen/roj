#ifndef APPHANDLE_HPP
#define APPHANDLE_HPP
#include <glad/glad.h>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <entt/entt.hpp>

#include <unordered_map>
#include <array>

#include "world.hpp"
#include "game/scenes/main_scene.hpp"

class AppHandle
{
private:
	GLFWwindow* m_window;
	GameWorld   m_world;
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
private:
	void initWindow();
	void update();
	void render();
public:
	AppHandle();
	void loop();
};
#endif // APPHANDLE_HPP
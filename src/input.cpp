#include "input.hpp"
#include "apphandle.hpp"

bool roj::getKeyPressed(roj::Keycode key)
{
	auto* window = static_cast<GLFWwindow*>(AppHandle::get()->getWindow());
	auto state = glfwGetKey(window, static_cast<int32_t>(key));
	return state == GLFW_PRESS;
}

bool roj::getMousePressed(roj::MouseButton key)
{
	auto* window = static_cast<GLFWwindow*>(AppHandle::get()->getWindow());
	auto state = glfwGetMouseButton(window, static_cast<int32_t>(key));
	return state == GLFW_PRESS;
}

void roj::enableCursor(bool state)
{
	auto* window = static_cast<GLFWwindow*>(AppHandle::get()->getWindow());
	glfwSetInputMode(window, GLFW_CURSOR, (state) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}
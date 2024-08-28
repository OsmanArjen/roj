#ifndef COMMON_COMPONENT_HPP
#define COMMON_COMPONENT_HPP
#include <glm/glm.hpp>
#include "shader.hpp"

namespace game
{
	struct Transform
	{
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };
		glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
	};

	struct Renderable
	{
		std::string model;
		std::string shader;
	};

	struct BasicRenderable
	{
		uint32_t VAO;
		std::string shader;
	};
}

#endif //COMMON_COMPONENT_HPP

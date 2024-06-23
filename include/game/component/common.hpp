#ifndef COMMON_COMPONENT_HPP
#define COMMON_COMPONENT_HPP
#include <glm/glm.hpp>
#include "shader.hpp"

namespace game
{
	struct Transform
	{
		glm::vec3 position;
		float scale;
	};

	struct Renderable
	{
		uint32_t modelIdx;
		std::string shader;
	};
}

#endif //COMMON_COMPONENT_HPP

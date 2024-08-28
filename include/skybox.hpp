#ifndef SKYBOX_HPP
#define SKYBOX_HPP
#include "utils.hpp"
namespace roj
{
struct Skybox
{
	uint32_t VAO = 0;
	uint32_t texture = 0;
	static Skybox create(std::vector<std::string> faces);
	void render();
};
}

#endif //-SKYBOX_HPP

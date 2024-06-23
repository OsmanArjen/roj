#ifndef GAME_RESOURCE_HPP
#include "model.hpp"
#include "shader.hpp"
#include <unordered_map>
struct GameResource
{
	std::vector<roj::model_t> m_models;
	std::unordered_map<std::string, roj::GLShaderObject> m_shaderObjects;
	std::unordered_map<std::string, uint32_t> m_textures;
}
#endif // !GAME_RESOURCE_HPP

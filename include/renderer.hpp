#ifndef RENDERER_HPP
#define RENDERER_HPP
#include "model.hpp"
#include "skinned_model.hpp"
#include "shader.hpp"
namespace game
{
	void renderModel(std::vector<roj::Mesh>& model, roj::GLShaderObject& shader);
	void renderModel(roj::SkinnedModel& model, roj::GLShaderObject& shader);
}
#endif //-RENDERER_HPP
